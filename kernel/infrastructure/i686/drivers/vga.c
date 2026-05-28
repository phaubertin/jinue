/*
 * Copyright (C) 2019 Philippe Aubertin.
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

typedef unsigned int vga_pos_t;

static log_reader_t log_reader;

/** base address of the VGA text video buffer */
static unsigned char *video_base_addr;

static vga_pos_t cursor_pos;

static void vga_clear(void) {
    unsigned int idx = 0;
    
    while( idx < (VGA_LINES * VGA_WIDTH * 2) ) {
        video_base_addr[idx++] = 0x20;
        video_base_addr[idx++] = VGA_COLOR_ERASE;
    }
}

static void write_misc_out_reg(uint8_t value) {
    outb(VGA_MISC_OUT_WR, value);
}

static void write_seq_reg(unsigned int index, uint8_t value) {
    outb(VGA_SEQ_ADDR, index);
    outb(VGA_SEQ_DATA, value);
}

static void write_crtc_reg(unsigned int index, uint8_t value) {
    /* unprotect or keep unprotected */
    if(index == 0x03) {
        value |= 0x80;
    }

    if(index == 0x11) {
        value &= ~0x80;
    }

    outb(VGA_CRTC_ADDR, index);
    outb(VGA_CRTC_DATA, value);
}

static uint8_t read_crtc_reg(unsigned int index) {
    outb(VGA_CRTC_ADDR, index);
    return inb(VGA_CRTC_DATA);
}

static void unprotect_crtc_regs(void) {
    /* write_crtc_reg() tweaks the values for the right CRTC registers to
     * unprotect. We only need to read and re-write the relevant registers. */
    write_crtc_reg(0x03, read_crtc_reg(0x03));
    write_crtc_reg(0x11, read_crtc_reg(0x11));
}

static void write_graphics_reg(unsigned int index, uint8_t value) {
    outb(VGA_GRAPHICS_ADDR, index);
    outb(VGA_GRAPHICS_DATA, value);
}

static void write_attr_reg(unsigned int index, uint8_t value) {
    /* The same register is used to set the address and value. We need to read
     * the input status 1 register first, which will cause the attribute
     * controller register to expect the index. */
    (void)inb(VGA_IN_STATUS1);
    outb(VGA_ATTR_WR_AND_ADDR, index);
    outb(VGA_ATTR_WR_AND_ADDR, value);
}

static void done_with_attr_regs(void) {
    /* Bit 5 in the attributes controller index register controls who can
     * access the palette registers: 0 for host and 1 for video memory. It
     * must be cleared to set the palette registers and then set again to
     * allow them to be used by the VGA hardware. It is set by this
     * function. */
    (void)inb(VGA_IN_STATUS1);
    outb(VGA_ATTR_WR_AND_ADDR, 0x20);
}

static vga_pos_t vga_get_cursor_pos(void) {
    return cursor_pos;
}

static void vga_set_cursor_pos(vga_pos_t pos) {
    cursor_pos = pos;

    unsigned char h = pos >> 8;
    unsigned char l = pos;

    write_crtc_reg(VGA_CRTC_CURSOR_HIGH, h);
    write_crtc_reg(VGA_CRTC_CURSOR_LOW, l);   
}

static int get_color(int loglevel) {
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

static void vga_scroll(void) {
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

static vga_pos_t vga_raw_putc(char c, vga_pos_t pos, int colour) {
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
        vga_scroll();
    }
    
    return pos;
}

static void do_log(const log_event_t *event) {
    int colour = get_color(event->loglevel);

    vga_pos_t pos  = vga_get_cursor_pos();

    for(int idx = 0; idx < event->length && event->message[idx] != '\0'; ++idx) {
        pos = vga_raw_putc(event->message[idx], pos, colour);
    }
    
    pos = vga_raw_putc('\n', pos, colour);

    vga_set_cursor_pos(pos);
}

void vga_init(const config_t *config) {
    if(! config->machine.vga_enable) {
        return;
    }

    if(!platform_is_vga_present()) {
        return;
    }

    /* Initialize VGA to 80x25 text mode
     *
     * Source for register values: https://files.osdev.org/mirrors/geezer/osd/graphics/modes.c
     * (See g_80x25_text and write_regs(), public domain, thanks to Chris Giese.)
     * 
     * The misc. output register must be written first because its LSB sets
     * the address of the CRTC registers (0x3dx in our case). */
    write_misc_out_reg(0x67);

    /* sequencer registers values */
    const uint8_t seq_regvals[] = {0x03, 0x00, 0x03, 0x00, 0x02};

    for(int idx = 0; idx < ARRAY_COUNT(seq_regvals); ++idx) {
        write_seq_reg(idx, seq_regvals[idx]);
    }

    /* CRTC registers */
    const uint8_t crtc_regvals[] = {
        0x5f, 0x4f, 0x50, 0x82, 0x55, 0x81, 0xbf, 0x1f,
        0x00, 0x4f, 0x0d, 0x0e, 0x00, 0x00, 0x00, 0x50,
        0x9c, 0x0e, 0x8f, 0x28, 0x1f, 0x96, 0xb9, 0xa3,
        0xff
    };

    unprotect_crtc_regs();

    for(int idx = 0; idx < ARRAY_COUNT(crtc_regvals); ++idx) {
        write_crtc_reg(idx, crtc_regvals[idx]);
    }

    /* Graphics controller registers */
    const uint8_t graphics_regvals[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x0e, 0x00,
	    0xff,
    };

    for(int idx = 0; idx < ARRAY_COUNT(graphics_regvals); ++idx) {
        write_graphics_reg(idx, graphics_regvals[idx]);
    }

    /* Attribute controller registers */
    const uint8_t attr_regvals[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
        0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
        0x0c, 0x00, 0x0f, 0x08, 0x00
    };

    for(int idx = 0; idx < ARRAY_COUNT(attr_regvals); ++idx) {
        write_attr_reg(idx, attr_regvals[idx]);
    }

    done_with_attr_regs();

    /* Move cursor to line 0 col 0 */
    vga_set_cursor_pos(0);

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
