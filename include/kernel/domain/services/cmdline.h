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

#ifndef JINUE_KERNEL_SERVICES_CMDLINE_H
#define JINUE_KERNEL_SERVICES_CMDLINE_H

#include <kernel/domain/services/asm/cmdline.h>
#include <kernel/types.h>

void cmdline_parse_options(config_t *config, const char *cmdline);

void cmdline_report_errors(void);

char *cmdline_write_arguments(char *buffer, const char *cmdline);

char *cmdline_write_environ(char *buffer, const char *cmdline);

size_t cmdline_count_arguments(const char *cmdline);

size_t cmdline_count_environ(const char *cmdline);

bool cmdline_match_integer(int *ivalue, const cmdline_token_t *value);

bool cmdline_match_enum(
        int                         *value,
        const cmdline_enum_def_t    *def,
        const cmdline_token_t       *token);

bool cmdline_match_boolean(bool *value, const cmdline_token_t *token);

void cmdline_report_options(const char *cmdline);

#endif
