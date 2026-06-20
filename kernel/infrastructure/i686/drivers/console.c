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

#include <kernel/domain/services/logging.h>
#include <kernel/infrastructure/i686/drivers/console.h>
#include <kernel/infrastructure/i686/boot_alloc.h>
#include <kernel/machine/asm/machine.h>
#include <kernel/utils/asm/ascii.h>
#include <kernel/utils/utils.h>
#include <ctype.h>
#include <stddef.h>
#include <stdint.h>

/** Initialize the console configuration and state
 * 
 * @param console the console state structure
 * @param width screen width in characters
 * @param height screen height in characters
 * @param write_func string write callback function
 * @param scroll_func text scrolling callback function
 */
void initialize_console(
    console_t               *console,
    unsigned int             width,
    unsigned int             height,
    console_write_func_t     write_func,
    console_scroll_func_t    scroll_func
){
    info("Initializing console with size %ux%u.", width, height);

    console->width = width;
    console->height = height;
    console->write_func = write_func;
    console->scroll_func = scroll_func;
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

    --console->row;

    console->scroll_func();    
}

/** Write a character to the console text buffer
 * 
 * @param console console state structure
 * @param c character to write
 * @param loglevel log level of written character
 */
static void write_character(console_t *console, char c, uint8_t loglevel) {
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
            /* The recusive call takes care of line wrapping if needed. There is at more one
             * level of recursion because the character is fixed to a space. */
            write_character(console, ' ', loglevel);
        }
    }
        break;
    
    default:
        if(isprint(c)) {
            console->write_func(c, loglevel);
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
 * @param loglevel log level of written text
 */
void console_write(console_t *console, const char *str, size_t length, uint8_t loglevel) {
    for(int idx = 0; idx < length; ++idx) {
        write_character(console, str[idx], loglevel);
    }

    write_character(console, '\n', loglevel);
}
