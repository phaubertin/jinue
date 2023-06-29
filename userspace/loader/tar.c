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
#include <string.h>
#include "tar.h"

static void compute_checksum(tar_header_t *header) {
    memset(header->chksum, ' ', sizeof(header->chksum));

    int chksm = 0;

    for(int idx = 0; idx < sizeof(tar_header_t); ++idx) {
        chksm += ((unsigned char *)header)[idx];
    }

    for(int pos = 0; pos < 6; ++pos) {
        header->chksum[5 - pos] = '0' + (chksm & 7);
        chksm >>= 3;
    }

    header->chksum[6] = 0;
    header->chksum[7] = ' ';
}

static bool is_checksum_valid(const tar_header_t *header) {
    tar_header_t copy;
    memcpy(&copy, header, sizeof(tar_header_t));
    compute_checksum(&copy);
    return memcmp(header->chksum, copy.chksum, sizeof(header->chksum)) == 0;
}

bool tar_is_header_valid(const tar_header_t *header) {
    const char *ustar = "ustar";

    if(strncmp(header->magic, ustar, strlen(ustar)) != 0) {
        return false;
    }

    return is_checksum_valid(header);
}
