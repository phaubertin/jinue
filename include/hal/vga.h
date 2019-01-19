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

#ifndef JINUE_HAL_VGA_H
#define JINUE_HAL_VGA_H

#define VGA_TEXT_VID_BASE           0xb8000
#define VGA_TEXT_VID_TOP            0xc0000
#define VGA_TEXT_VID_SIZE           (VGA_TEXT_VID_TOP - VGA_TEXT_VID_BASE)
#define VGA_MISC_OUT_WR             0x3c2
#define VGA_MISC_OUT_RD             0x3cc
#define VGA_CRTC_ADDR               0x3d4
#define VGA_CRTC_DATA               0x3d5

#define VGA_FB_FLAG_ACTIVE    1

#define VGA_COLOR_BLACK             0x00
#define VGA_COLOR_BLUE              0x01
#define VGA_COLOR_GREEN             0x02
#define VGA_COLOR_CYAN              0x03
#define VGA_COLOR_RED               0x04
#define VGA_COLOR_MAGENTA           0x05
#define VGA_COLOR_BROWN             0x06
#define VGA_COLOR_WHITE             0x07
#define VGA_COLOR_GRAY              0x08
#define VGA_COLOR_BRIGHTBLUE        0x09
#define VGA_COLOR_BRIGHTGREEN       0x0a
#define VGA_COLOR_BRIGHTCYAN        0x0b
#define VGA_COLOR_BRIGHTRED         0x0c
#define VGA_COLOR_BRIGHTMAGENTA     0x0d
#define VGA_COLOR_YELLOW            0x0e
#define VGA_COLOR_BRIGHTWHITE       0x0f
#define VGA_COLOR_ERASE             VGA_COLOR_RED

#define VGA_LINES                   25
#define VGA_WIDTH                   80
#define VGA_TAB_WIDTH               8

#define VGA_LINE(x)                 ((x) / (VGA_WIDTH))
#define VGA_COL(x)                  ((x) % (VGA_WIDTH))


typedef unsigned int vga_pos_t;


void vga_init(void);

void vga_set_base_addr(void *base_addr);

void vga_clear(void);

void vga_print(const char *message, int colour);

void vga_printn(const char *message, unsigned int n, int colour);

void vga_putc(char c, int colour);

void vga_scroll(void);

vga_pos_t vga_get_cursor_pos(void);

void vga_set_cursor_pos(vga_pos_t pos);

#endif
