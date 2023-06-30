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

#include <jinue/util.h>
#include <stdlib.h>
#include <zlib.h>
#include "gzip.h"

struct gzip_context {
    z_stream strm;
};

static void *zalloc(void *unused, uInt items, uInt size) {
    return malloc(items * size);
}

static void zfree(void *unused, void *address) {
    free(address);
}

bool gzip_is_header_valid(const void *compressed) {
    const unsigned char *header = compressed;
    return header[0] == 0x1f && header[1] == 0x8b && header[2] == 0x08;
}

int gzip_initialize(gzip_context_t *ctx, const void *compressed, size_t size) {
    z_stream *strm = &ctx->strm;
    strm->zalloc = zalloc;
    strm->zfree = zfree;
    strm->opaque = NULL;
    strm->next_in = compressed;
    strm->avail_in = size;

    int status = inflateInit2(strm, 16);

    if(status != Z_OK) {
        jinue_error(
                "error: zlib initialization failed: %s",
                strm->msg == NULL ? "(no message)" : strm->msg
        );
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int gzip_inflate(gzip_context_t *ctx, void *buffer, size_t size) {
    z_stream *strm = &ctx->strm;
    strm->next_out = buffer;
    strm->avail_out = size;

    int status = inflate(strm, Z_SYNC_FLUSH);

    if(status != Z_OK) {
        jinue_error(
                "error: zlib could not inflate: %s",
                strm->msg == NULL ? "(no message)" : strm->msg
        );
        return EXIT_FAILURE;
    }

    if(strm->avail_out != 0) {
        jinue_error(
                "error: zlib could only decompress %zu of %zu bytes requested",
                size - strm->avail_out,
                size
        );
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void gzip_finalize(gzip_context_t *ctx) {
    (void)inflateEnd(&ctx->strm);
}
