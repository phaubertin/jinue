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

#include <jinue/shared/syscall.h>
#include <jinue/shared/vm.h>
#include <ascii.h>
#include <hal/io.h>
#include <hal/vga.h>
#include <hal/vm.h>

/** base address of the VGA text video buffer */
static unsigned char *video_base_addr = (void *)VGA_TEXT_VID_BASE;

static vga_pos_t vga_raw_putc(char c, vga_pos_t pos, int colour);

void vga_init(void) {
    unsigned char data;
    
    /* Set address select bit in a known state: CRTC regs at 0x3dx */
    data = inb(VGA_MISC_OUT_RD);
    data |= 1;
    outb(VGA_MISC_OUT_WR, data);

    /* Move cursor to line 0 col 0 */
    outb(VGA_CRTC_ADDR, 0x0e);
    outb(VGA_CRTC_DATA, 0x0);    
    outb(VGA_CRTC_ADDR, 0x0f);
    outb(VGA_CRTC_DATA, 0x0);
    
    /* Clear the screen */
    vga_clear();
}

void vga_set_base_addr(void *base_addr) {
    video_base_addr = base_addr;
}

void vga_clear(void) {
    unsigned int idx = 0;
    
    while( idx < (VGA_LINES * VGA_WIDTH * 2) ) {
        video_base_addr[idx++] = 0x20;
        video_base_addr[idx++] = VGA_COLOR_ERASE;
    }
}

void vga_scroll(void) {
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

vga_pos_t vga_get_cursor_pos(void) {
    unsigned char h, l;
    
    outb(VGA_CRTC_ADDR, 0x0e);
    h = inb(VGA_CRTC_DATA);
    outb(VGA_CRTC_ADDR, 0x0f);
    l = inb(VGA_CRTC_DATA);
    
    return (h << 8) | l;
}

void vga_set_cursor_pos(vga_pos_t pos) {
    unsigned char h = pos >> 8;
    unsigned char l = pos;
    
    outb(VGA_CRTC_ADDR, 0x0e);
    outb(VGA_CRTC_DATA, h);
    outb(VGA_CRTC_ADDR, 0x0f);
    outb(VGA_CRTC_DATA, l);    
}

static int get_colour(int loglevel) {
    switch(loglevel) {
    case JINUE_PUTS_LOGLEVEL_INFO:
        return VGA_COLOR_BRIGHTGREEN;
    case JINUE_PUTS_LOGLEVEL_WARNING:
        return VGA_COLOR_YELLOW;
    case JINUE_PUTS_LOGLEVEL_ERROR:
        return VGA_COLOR_RED;
    default:
        return VGA_COLOR_GRAY;
    }
}

void vga_printn(int loglevel, const char *message, size_t n) {
    int colour = get_colour(loglevel);

    unsigned short int pos  = vga_get_cursor_pos();

    for(int idx = 0; idx < n && message[idx] != '\0'; ++idx) {
        pos = vga_raw_putc(message[idx], pos, colour);
    }
    
    pos = vga_raw_putc('\n', pos, colour);

    vga_set_cursor_pos(pos);
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
