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

#ifndef JINUE_KERNEL_INFRASTRUCTURE_I686_DRIVERS_CONSOLE_H_
#define JINUE_KERNEL_INFRASTRUCTURE_I686_DRIVERS_CONSOLE_H_

#include <kernel/infrastructure/i686/drivers/asm/console.h>
#include <kernel/interface/i686/types.h>
#include <stddef.h>
#include <stdint.h>

typedef void (*console_write_func_t)(unsigned char c, uint8_t loglevel);

typedef void (*console_scroll_func_t)(void);

typedef struct {
    unsigned int            width;
    unsigned int            height;
    unsigned int            row;
    unsigned int            col;
    console_write_func_t    write_func;
    console_scroll_func_t   scroll_func;
} console_t;

void initialize_console(
    console_t               *console,
    unsigned int             width,
    unsigned int             height,
    console_write_func_t     write_func,
    console_scroll_func_t    scroll_func
);

size_t console_text_buffer_size(unsigned int width, unsigned int height);

void console_write(console_t *console, const char *str, size_t length, uint8_t loglevel);

#endif
