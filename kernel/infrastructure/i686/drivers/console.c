/*
 * Copyright (C) 2026 Philippe Aubertin.
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

/* This file contains code shared by the VGA (text) and video framebuffer
 * drivers. For the VGA driver, the underlying buffer we are manipulating in
 * this file is the actual VGA hardware buffer at 0xb8000 while, for the
 * framebuffer driver, it is a temporary text buffer that is then used as the
 * for rendering the characters in the video framebuffer.
 * 
 * The format is imposed by VGA, i.e. two bytes per character where the first
 * byte is the character code and the second byte is the colour. */

#include <kernel/domain/services/logging.h>
#include <kernel/infrastructure/i686/drivers/console.h>
#include <kernel/infrastructure/i686/boot_alloc.h>
#include <kernel/machine/asm/machine.h>
#include <kernel/utils/asm/ascii.h>
#include <kernel/utils/utils.h>
#include <stddef.h>
#include <stdint.h>

/** Initialize the console configuration and state
 * 
 * @param console the console state structure
 * @param buffer pointer to the buffer, mapped
 * @param width screen width in characters
 * @param height screen height in characters
 * @param erase_colour colour to erase with when erasing and scrolling
 */
void initialize_console(
    console_t       *console,
    unsigned char   *buffer,
    unsigned int     width,
    unsigned int     height,
    uint8_t          erase_colour
) {
    info("Initializing console with size %ux%u.", width, height);

    console->width = width;
    console->height = height;
    console->buffer = buffer;
    console->erase_colour = erase_colour;
    console->row = 0;
    console->col = 0;
}

/** Allocate text buffer and initialize the console configuration and state
 * 
 * The text buffer is allocated using the boot-time page allocator, so this
 * function must only be called during boot-time initialization.
 * 
 * @param console the console state structure
 * @param width screen width in characters
 * @param height screen height in characters
 * @param erase_colour colour to erase with when erasing and scrolling
 */
void allocate_console(
    console_t       *console,
    boot_alloc_t    *boot_alloc,
    unsigned int     width,
    unsigned int     height,
    uint8_t          erase_colour
) {
    size_t size = ALIGN_END(2 * width * height, PAGE_SIZE);

    unsigned char *buffer = boot_page_alloc_n(boot_alloc, size / PAGE_SIZE);

    initialize_console(console, buffer, width, height, erase_colour);
}

/** Erase the console and set position to home
 * 
 * @param console the console state structure
 */
void erase_console(console_t *console) {
    unsigned int idx = 0;

    size_t size = 2 * console->width * console->height;
    
    while(idx < size) {
        console->buffer[idx++] = ' ';
        console->buffer[idx++] = console->erase_colour;
    }

    console->row = 0;
    console->col = 0;
}

/** Perform wrapping if current column is out of the screen
 * 
 * @param console the console state structure
 */
static void wrap_text(console_t *console) {
    if(console->col < console->width) {
        return;
    }

    console->col -= console->width;
    console->row += 1;
}

/** Scroll if current row is out of the screen
 * 
 * @param console the console state structure
 */
static void scroll_text(console_t *console) {
    if(console->row < console->height) {
        return;
    }

    unsigned char *di = console->buffer;
    unsigned char *si = &console->buffer[2 * console->width];
    unsigned int idx;
    
    for(idx = 0; idx < 2 * console->width * (console->height - 1); ++idx) {
        *(di++) = *(si++);
    }
    
    for(idx = 0; idx < console->width; ++idx) {
        *(di++) = ' ';
        *(di++) = console->erase_colour;
    }

    --console->row;
}

/** Write a character to the console text buffer
 * 
 * @param console console state structure
 * @param c character to write
 * @param colour colour number
 */
static void write_character(console_t *console, char c, uint8_t colour) {
    switch(c) {
    /* linefeed - actually does cr + lf */
    case CHAR_LF:
        console->col = 0;
        console->row += 1;
        break;
    
    /* tab */
    case CHAR_HT:
    {
        const unsigned int skip = CONSOLE_TAB_WIDTH - (console->col % CONSOLE_TAB_WIDTH);

        for(int idx = 0; idx < skip; ++idx) {
            write_character(console, ' ', colour);
        }
    }
        break;
    
    default:
        if(c >= 0x20 && c < 0x7f) {
            const unsigned int offset = console->width * console->row + console->col;
            console->buffer[2 * offset] = (unsigned char)c;
            console->buffer[2 * offset + 1] = colour;
            
            console->col += 1;
            wrap_text(console);
        }
    }
    
    scroll_text(console);
}

/** Write a string to the console text buffer
 * 
 * @param console console state structure
 * @param str string to write
 * @param length length of the string
 * @param colour colour number of written text
 */
void console_write(console_t *console, const char *str, size_t length, uint8_t colour) {
    for(int idx = 0; idx < length; ++idx) {
        write_character(console, str[idx], colour);
    }

    write_character(console, '\n', colour);
}
