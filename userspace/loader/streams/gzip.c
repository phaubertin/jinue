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
#include <stdbool.h>
#include <stdlib.h>
#include <zlib.h>
#include "gzip.h"

struct state {
    const void  *addr;
    size_t       size;
    z_stream     zstrm;
};

static int read(stream_t *stream, void *dest, size_t size) {
    struct state *state = stream->state;

    z_stream *zstrm     = &state->zstrm;
    zstrm->next_out     = dest;
    zstrm->avail_out    = size;

    int status = inflate(zstrm, Z_SYNC_FLUSH);

    if(status != Z_OK) {
        jinue_error(
                "error: zlib could not inflate: %s",
                zstrm->msg == NULL ? "(no message)" : zstrm->msg
        );
        return STREAM_ERROR;
    }

    if(zstrm->avail_out != 0) {
        jinue_error(
                "error: zlib could only decompress %zu of %zu bytes requested",
                size - zstrm->avail_out,
                size
        );
        return STREAM_ERROR;
    }

    return STREAM_SUCCESS;
}

static void *zalloc(void *unused, uInt items, uInt size) {
    return malloc(items * size);
}

static void zfree(void *unused, void *address) {
    free(address);
}

static int initialize_zlib_stream(z_stream *zstrm, const void *addr, size_t size) {
    zstrm->zalloc   = zalloc;
    zstrm->zfree    = zfree;
    zstrm->opaque   = NULL;
    zstrm->next_in  = addr;
    zstrm->avail_in = size;

    int status = inflateInit2(zstrm, 16);

    if(status != Z_OK) {
        return STREAM_ERROR;
    }

    return STREAM_SUCCESS;
}

static void finalize_zlib_stream(z_stream *zstrm) {
    (void)inflateEnd(zstrm);
}

static int reset(stream_t *stream) {
    struct state *oldstate = stream->state;
    struct state *newstate = malloc(sizeof(struct state));

    if(newstate == NULL) {
        return STREAM_ERROR;
    }

    z_stream *zstrm = &newstate->zstrm;
    int status = initialize_zlib_stream(zstrm, oldstate->addr, oldstate->size);

    if(status != STREAM_SUCCESS) {
        jinue_error(
                "error: zlib initialization failed (on reset): %s",
                zstrm->msg == NULL ? "(no message)" : zstrm->msg
        );
        free(newstate);
        return STREAM_ERROR;
    }

    newstate->addr  = oldstate->addr;
    newstate->size  = oldstate->size;

    stream->state   = newstate;

    finalize_zlib_stream(&oldstate->zstrm);
    free(oldstate);

    return STREAM_SUCCESS;
}

static void finalize(stream_t *stream) {
    struct state *state = stream->state;
    finalize_zlib_stream(&state->zstrm);
    free(state);
}

static bool is_header_valid(const void *addr, size_t size) {
    if(size < 10) {
        return false;
    }

    const unsigned char *header = addr;
    return header[0] == 0x1f && header[1] == 0x8b && header[2] == 0x08;
}

int gzip_stream_initialize(stream_t *stream, const void *addr, size_t size) {
    if(!is_header_valid(addr, size)) {
        return STREAM_FORMAT;
    }

    struct state *state = malloc(sizeof(struct state));

    if(state == NULL) {
        return STREAM_ERROR;
    }

    z_stream *zstrm = &state->zstrm;
    int status = initialize_zlib_stream(zstrm, addr, size);

    if(status != STREAM_SUCCESS) {
        jinue_error(
                "error: zlib initialization failed: %s",
                zstrm->msg == NULL ? "(no message)" : zstrm->msg
        );
        free(state);
        return STREAM_ERROR;
    }

    state->addr             = addr;
    state->size             = size;

    stream->state           = state;
    stream->ops.read        = read;
    stream->ops.reset       = reset;
    stream->ops.finalize    = finalize;

    return STREAM_SUCCESS;
}
