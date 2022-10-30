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

#include <jinue/logging.h>
#include <jinue/syscall.h>
#include <stdio.h>
#include <string.h>

static void log_message(int loglevel, const char *restrict format, va_list args) {
    /* TODO figure out the right size */
    char buffer[120];

    /* TODO (maybe?) append (...) when the string is cropped */
    vsnprintf(buffer, sizeof(buffer), format, args);

    /* There are two situations where the PUTS system call can fail:
     *
     * - When the string is too long. This function crops the string to ensure
     *   this doesn't happen.
     * - When the value of the loglevel argument is not recognized. This
     *   function is static and is only called by utility functions that pass
     *   a specific log level.
     *
     * For these reasons, this function won't fail, so we can pass NULL as
     * the error number pointer and not return anything. */
    jinue_puts(loglevel, buffer, strlen(buffer), NULL);
}

void jinue_vinfo(const char *restrict format, va_list args) {
    log_message('I', format, args);
}

void jinue_vwarning(const char *restrict format, va_list args) {
    log_message('W', format, args);
}

void jinue_verror(const char *restrict format, va_list args) {
    log_message('E', format, args);
}

void jinue_info(const char *restrict format, ...) {
    va_list args;

    va_start(args, format);

    jinue_vinfo(format, args);

    va_end(args);
}

void jinue_warning(const char *restrict format, ...) {
    va_list args;

    va_start(args, format);

    jinue_vwarning(format, args);

    va_end(args);
}

void jinue_error(const char *restrict format, ...) {
    va_list args;

    va_start(args, format);

    jinue_verror(format, args);

    va_end(args);
}


