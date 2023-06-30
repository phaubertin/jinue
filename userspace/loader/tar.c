/*
 * Copyright (C) 2023 Philippe Aubertin.
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

#include <jinue/initrd.h>
#include <jinue/util.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "tar.h"

static bool is_octal_digit(int c) {
    return c >= '0' && c <= '7';
}

static int octal_digit_value(int c) {
    return c - '0';
}

static int64_t decode_octal_field(const char *field, size_t length) {
    const char *const end = field + length;

    while(field < end && *field == ' ') {
        ++field;
    }

    int64_t value = 0;

    while(field < end && is_octal_digit(*field)) {
        value <<= 3;
        value += octal_digit_value(*field);
        ++field;
    }

    return value;
}

static bool is_checksum_valid(const tar_header_t *header) {
    int checksum = ' ' * sizeof(header->chksum);

    for(int idx = 0; idx < sizeof(tar_header_t); ++idx) {
        checksum += ((unsigned char *)header)[idx];
    }

    for(int idx = 0; idx < sizeof(header->chksum); ++idx) {
        checksum -= header->chksum[idx];
    }

    return checksum == decode_octal_field(header->chksum, sizeof(header->chksum));
}

bool tar_is_header_valid(const tar_header_t *header) {
    const char *ustar = "ustar";

    if(strncmp(header->magic, ustar, strlen(ustar)) != 0) {
        return false;
    }

    return is_checksum_valid(header);
}

int tar_extract(gzip_context_t *gzip_context) {
    unsigned char buffer[512];

    int status = gzip_inflate(gzip_context, buffer, sizeof(buffer));

    if(status != EXIT_SUCCESS) {
        return status;
    }

    if(! tar_is_header_valid((const tar_header_t *)buffer)) {
        jinue_error("error: compressed data is not a tar archive (bad signature or checksum).");
        return EXIT_FAILURE;
    }

    jinue_info("compressed data is a tar archive");

    /* TODO extract tar archive */

    return EXIT_SUCCESS;
}
