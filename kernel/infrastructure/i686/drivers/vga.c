/*
 * Copyright (C) 2019-2026 Philippe Aubertin.
 * All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the author nor the names of other contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <jinue/shared/asm/logging.h>
#include <jinue/shared/asm/mman.h>
#include <kernel/domain/services/logging.h>
#include <kernel/domain/services/mman.h>
#include <kernel/infrastructure/i686/drivers/vga.h>
#include <kernel/infrastructure/i686/isa/io.h>
#include <kernel/infrastructure/i686/pmap/pmap.h>
#include <kernel/infrastructure/i686/platform.h>
#include <kernel/utils/asm/ascii.h>
#include <kernel/utils/pmap.h>
#include <kernel/utils/utils.h>

/** type for the current linear position in the text buffer */
typedef unsigned int vga_pos_t;

/** current cursor/next write position in the text buffer
 * 
 *  Position in number of characters (not bytes, there are two bytes per
 *  character in the text buffer because of the attributes).
 */
static vga_pos_t cursor_pos;

/** log ring buffer reader */
static log_reader_t log_reader;

/** base address of the VGA text video buffer */
static unsigned char *video_base_addr;

/** Erase the VGA text buffer. */
static void vga_clear(void) {
    unsigned int idx = 0;
    
    while( idx < (VGA_LINES * VGA_WIDTH * 2) ) {
        video_base_addr[idx++] = 0x20;
        video_base_addr[idx++] = VGA_COLOR_ERASE;
    }
}

/** Write a value to the misc. output register.
 * 
 * @param value value to write in register
 */
static void write_misc_out_reg(uint8_t value) {
    outb(VGA_MISC_OUT_WR, value);
}

/** Read a value from the misc. output register.
 * 
 * @return register value
 */
static uint8_t read_misc_out_reg(void) {
    return inb(VGA_MISC_OUT_RD);
}

/** Write a value to one of the CRT Controller (CRTC) registers.
 * 
 * When writing to register index 3, this function automatically sets the test
 * bit (bit 7) that must be written to one.
 * 
 * When writing to register index 17 (0x11), this function automatically clears
 * bit 7 to unprotect CRTC registers witth indexes 0-7.
 * 
 * @param index index of register to write to
 * @param value value to write in register
 */
static void write_crtc_reg(unsigned int index, uint8_t value) {
    outb(VGA_CRTC_ADDR, index);
    outb(VGA_CRTC_DATA, value);
}

/** Get the current position of the text cursor.
 * 
 * The position is obtained from the shadow variable.
 * 
 * @return the current linear position of the text cursor
 */
static vga_pos_t get_cursor_position(void) {
    return cursor_pos;
}

/** Set the position of the text cursor.
 * 
 * This function updates both the the relevant hardware register and the shadow
 * variable.
 * 
 * @param pos linear position of the text cursor
 */
static void set_cursor_position(vga_pos_t pos) {
    cursor_pos = pos;

    unsigned char h = pos >> 8;
    unsigned char l = pos;

    write_crtc_reg(VGA_CRTC_CURSOR_HIGH, h);
    write_crtc_reg(VGA_CRTC_CURSOR_LOW, l);   
}

/** Map a log level to the appropriate VGA colour number.
 * 
 * @param loglevel the log level
 * @return VGA colour number
 */
static int map_color(int loglevel) {
    switch(loglevel) {
    case JINUE_LOG_LEVEL_EMERGENCY:
    case JINUE_LOG_LEVEL_ALERT:
    case JINUE_LOG_LEVEL_CRITICAL:
        return VGA_COLOR_RED;
    case JINUE_LOG_LEVEL_ERROR:
        return VGA_COLOR_BRIGHTRED;
    case JINUE_LOG_LEVEL_WARNING:
        return VGA_COLOR_YELLOW;
    case JINUE_LOG_LEVEL_NOTICE:
    case JINUE_LOG_LEVEL_INFO:
        return VGA_COLOR_BRIGHTGREEN;
    default:
        return VGA_COLOR_GRAY;
    }
}

/** Initialize registers */
static void initialize_registers(void) {
    /* Set address select bit in a known state: CRTC regs at 0x3dx */
    write_misc_out_reg(read_misc_out_reg() | 1);
}

/** Scroll the text buffer by one line. */
static void scroll_text(void) {
    unsigned char *di = video_base_addr;
    unsigned char *si = video_base_addr + 2 * VGA_WIDTH;
    unsigned int idx;
    
    for(idx = 0; idx < 2 * VGA_WIDTH * (VGA_LINES - 1); ++idx) {
        *(di++) = *(si++);
    }
    
    for(idx = 0; idx < VGA_WIDTH; ++idx) {
        *(di++) = 0x20;
        *(di++) = VGA_COLOR_ERASE;
    }
}

/** Write a character in the text buffer
 * 
 * @param c character to write
 * @param pos position at which to write the character
 * @param colour color number
 */
static vga_pos_t write_character(char c, vga_pos_t pos, int colour) {
    const vga_pos_t limit = VGA_WIDTH * VGA_LINES;
    
    switch(c) {
    /* backspace */
    case CHAR_BS:
        if( VGA_COL(pos) > 0 ) {
            --pos;
        }
        break;
    
    /* linefeed - actually does cr + lf */
    case CHAR_LF:
        pos = VGA_WIDTH * (VGA_LINE(pos) + 1);
        break;
    
    /* carriage return */
    case CHAR_CR:
        pos = VGA_WIDTH * VGA_LINE(pos);
        break;
    
    /* tab */
    case CHAR_HT:
        pos -= pos % VGA_TAB_WIDTH;
        pos += VGA_TAB_WIDTH;
        break;
    
    default:
        if(c >= 0x20) {
            video_base_addr[2 * pos]     = (unsigned char)c;
            video_base_addr[2 * pos + 1] = colour;
            ++pos;
        }
    }
    
    if(pos >= limit) {
        pos -= VGA_WIDTH;
        scroll_text();
    }
    
    return pos;
}

/** Logging callback function.
 * 
 * This function is registered as the logging callback function and is called
 * by the logging infrastructure for each logging event.
 * 
 * @param event logging event
*/
static void do_log(const log_event_t *event) {
    int colour = map_color(event->loglevel);

    vga_pos_t pos  = get_cursor_position();

    for(int idx = 0; idx < event->length && event->message[idx] != '\0'; ++idx) {
        pos = write_character(event->message[idx], pos, colour);
    }
    
    pos = write_character('\n', pos, colour);

    set_cursor_position(pos);
}

/** Initialize VGA for logging.
 * 
 * @param config kernel configuration
 */
void vga_init(const config_t *config) {
    if(! config->machine.vga_enable) {
        return;
    }

    if(!platform_is_vga_present()) {
        return;
    }

    initialize_registers();

    /* Move cursor to line 0 col 0 */
    set_cursor_position(0);

    video_base_addr = map_in_kernel(
        VGA_TEXT_VID_BASE,
        VGA_TEXT_VID_TOP - VGA_TEXT_VID_BASE,
        JINUE_PROT_READ | JINUE_PROT_WRITE
    );
    
    /* Clear the screen */
    vga_clear();

    initialize_log_reader(&log_reader, do_log);

    register_log_reader(&log_reader);
}
