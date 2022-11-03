/*
 * Copyright (C) 2022 Philippe Aubertin.
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
#include <kernel/hal/serial.h>
#include <kernel/hal/vga.h>
#include <kernel/cmdline.h>
#include <kernel/logging.h>
#include <stdio.h>

void logging_init(const cmdline_opts_t *cmdline_opts) {
    if(cmdline_opts->vga_enable) {
        vga_init();
    }
    if(cmdline_opts->serial_enable) {
        serial_init(cmdline_opts->serial_ioport, cmdline_opts->serial_baud_rate);
    }
}

static void log_message(int loglevel, const char *restrict format, va_list args) {
    /* TODO implement kernel logs ring buffer and get rid of this buffer
     *
     * This is static so it does not take a big chunk of the thread's stack. The
     * downside is obviously that this function is not reentrant. */
    static char message[JINUE_PUTS_MAX_LENGTH + 1];

    size_t length = vsnprintf(message, sizeof(message), format, args);
    logging_add_message(loglevel, message, length);
}

void logging_add_message(int loglevel, const char *message, size_t n) {
    const cmdline_opts_t *cmdline_opts = cmdline_get_options();

    if(cmdline_opts->vga_enable) {
        vga_printn(loglevel, message, n);
    }
    if(cmdline_opts->serial_enable) {
        serial_printn(cmdline_opts->serial_ioport, message, n);
    }
}

void info(const char *restrict format, ...) {
    va_list args;

    va_start(args, format);

    log_message(JINUE_PUTS_LOGLEVEL_INFO, format, args);

    va_end(args);
}

void warning(const char *restrict format, ...) {
    va_list args;

    va_start(args, format);

    log_message(JINUE_PUTS_LOGLEVEL_WARNING, format, args);

    va_end(args);
}

void error(const char *restrict format, ...) {
    va_list args;

    va_start(args, format);

    log_message(JINUE_PUTS_LOGLEVEL_ERROR, format, args);

    va_end(args);
}
