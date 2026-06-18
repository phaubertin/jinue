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
#include <kernel/infrastructure/i686/asm/video.h>
#include <kernel/infrastructure/i686/drivers/console.h>
#include <kernel/infrastructure/i686/drivers/vga.h>
#include <kernel/infrastructure/i686/isa/io.h>
#include <kernel/infrastructure/i686/pmap/pmap.h>
#include <kernel/infrastructure/i686/platform.h>
#include <kernel/utils/asm/ascii.h>
#include <kernel/utils/pmap.h>
#include <kernel/utils/utils.h>
#include <inttypes.h>

/** console abstraction */
static console_t console;

/** log ring buffer reader */
static log_reader_t log_reader;

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

/** Set the position of the text cursor. */
static void update_cursor_position(void) {
    unsigned int offset = console.width * console.row + console.col;

    unsigned char h = offset >> 8;
    unsigned char l = offset;

    write_crtc_reg(VGA_CRTC_CURSOR_HIGH, h);
    write_crtc_reg(VGA_CRTC_CURSOR_LOW, l);
}

/** Map a log level to the appropriate VGA colour number.
 * 
 * @param loglevel the log level
 * @return VGA colour number
 */
static uint8_t map_colour(int loglevel) {
    switch(loglevel) {
    case JINUE_LOG_LEVEL_EMERGENCY:
    case JINUE_LOG_LEVEL_ALERT:
    case JINUE_LOG_LEVEL_CRITICAL:
        return VGA_COLOUR_RED;
    case JINUE_LOG_LEVEL_ERROR:
        return VGA_COLOUR_BRIGHTRED;
    case JINUE_LOG_LEVEL_WARNING:
        return VGA_COLOUR_YELLOW;
    case JINUE_LOG_LEVEL_NOTICE:
    case JINUE_LOG_LEVEL_INFO:
        return VGA_COLOUR_BRIGHTGREEN;
    default:
        return VGA_COLOUR_GRAY;
    }
}

/** Initialize registers */
static void initialize_registers(void) {
    /* Set address select bit in a known state: CRTC regs at 0x3dx */
    write_misc_out_reg(read_misc_out_reg() | 1);
}

/** Logging callback function.
 * 
 * This function is registered as the logging callback function and is called
 * by the logging infrastructure for each logging event.
 * 
 * @param event logging event
*/
static void do_log(const log_event_t *event) {
    uint8_t colour = map_colour(event->loglevel);

    console_write(&console, event->message, event->length, colour);

    update_cursor_position();
}

/** Initialize VGA for logging.
 * 
 * @param config kernel configuration
 * @param bootinfo boot information structure
 */
void vga_init(const config_t *config, const bootinfo_t *bootinfo) {
    if(! config->machine.vga_enable) {
        return;
    }

    if(!platform_is_vga_present()) {
        return;
    }

    if(platform_get_video_type() != VIDEO_TYPE_TEXT) {
        return;
    }

    info("Initializing VGA text mode 80x25.");

    initialize_registers();

    unsigned char *video_base_addr = map_in_kernel(
        VGA_TEXT_VID_BASE,
        VGA_TEXT_VID_TOP - VGA_TEXT_VID_BASE,
        JINUE_PROT_READ | JINUE_PROT_WRITE
    );

    initialize_console(&console, video_base_addr, VGA_WIDTH, VGA_LINES, VGA_COLOUR_ERASE);

    erase_console(&console);

    update_cursor_position();

    initialize_log_reader(&log_reader, do_log);

    register_log_reader(&log_reader);
}
