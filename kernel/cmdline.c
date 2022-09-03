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

#include <cmdline.h>
#include <console.h>

typedef struct {
    const char  *name;
    int          enum_value;
} enum_def_t;

typedef enum {
    CMDLINE_OPT_NAME_PAE,
    CMDLINE_OPT_NAME_SERIAL_ENABLED,
    CMDLINE_OPT_NAME_SERIAL_BAUD_RATE,
    CMDLINE_OPT_NAME_SERIAL_IOPORT,
    CMDLINE_OPT_NAME_SERIAL_PORTN,
    CMDLINE_OPT_NAME_VGA_ENABLED,
} cmdline_opt_names_t;

static const enum_def_t opt_names[] = {
    {"pae",                 CMDLINE_OPT_NAME_PAE},
    {"serial_enabled",      CMDLINE_OPT_NAME_SERIAL_ENABLED},
    {"serial_baud_rate",    CMDLINE_OPT_NAME_SERIAL_BAUD_RATE},
    {"serial_ioport",       CMDLINE_OPT_NAME_SERIAL_IOPORT},
    {"serial_portn",        CMDLINE_OPT_NAME_SERIAL_PORTN},
    {"vga_enabled",         CMDLINE_OPT_NAME_VGA_ENABLED},
    {NULL, 0}
};

static const enum_def_t opt_pae_names[] = {
    {"auto",        CMDLINE_OPT_PAE_AUTO},
    {"disable",     CMDLINE_OPT_PAE_DISABLE},
    {"require",     CMDLINE_OPT_PAE_REQUIRE},
    {NULL, 0}
};

static const enum_def_t bool_names[] = {
    {"true",    true},
    {"yes",     true},
    {"enable",  true},
    {"1",       true},
    {"false",   false},
    {"no",      false},
    {"disable", false},
    {"0",       false},
    {NULL, 0}
};

static cmdline_opts_t cmdline_options = {
    .pae                = CMDLINE_OPT_PAE_AUTO,
    .serial_enabled     = false,
    .serial_baud_rate   = CONSOLE_SERIAL_BAUD_RATE,
    .serial_ioport      = CONSOLE_SERIAL_IOPORT,
    .vga_enabled        = true,
};

const cmdline_opts_t *cmdline_get_options(void) {
    return &cmdline_options;
}
