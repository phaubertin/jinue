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

#include <hal/serial.h>
#include <hal/vga.h>
#include <cmdline.h>
#include <console.h>
#include <string.h>


void console_init(const cmdline_opts_t *cmdline_opts) {
    /* TODO validate arguments before using them */
    if(cmdline_opts->vga_enable) {
        vga_init();
    }
    if(cmdline_opts->serial_enable) {
        serial_init(cmdline_opts->serial_ioport, cmdline_opts->serial_baud_rate);
    }
}

void console_printn(const char *message, unsigned int n, int colour) {
    const cmdline_opts_t *cmdline_opts = cmdline_get_options();

    if(cmdline_opts->vga_enable) {
        vga_printn(message, n, colour);
    }
    if(cmdline_opts->serial_enable) {
        serial_printn(cmdline_opts->serial_ioport, message, n);
    }
}

void console_putc(char c, int colour) {
    const cmdline_opts_t *cmdline_opts = cmdline_get_options();

    if(cmdline_opts->vga_enable) {
        vga_putc(c, colour);
    }
    if(cmdline_opts->serial_enable) {
        serial_putc(cmdline_opts->serial_ioport, c);
    }
}

void console_print(const char *message, int colour) {
    console_printn(message, strlen(message), colour);
}
