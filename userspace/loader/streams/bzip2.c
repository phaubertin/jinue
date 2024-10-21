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

#include <jinue/utils.h>
#include <bzlib.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "bzip2.h"

struct state {
    const void  *addr;
    size_t       size;
    bz_stream    bz2strm;
};

static const char block_magic[] = {0x31, 0x41, 0x59, 0x26, 0x53, 0x59};

/* libbz2 has a dependency on this function, by name. */
void bz_internal_error(int errcode) {
    jinue_error("bz_internal_error(%s)", errcode);
}

static const char *bzip2_strerror(int status) {
    switch(status) {
        case BZ_SEQUENCE_ERROR:
            return "sequence error";
        case BZ_PARAM_ERROR:
            return "invalid parameter";
        case BZ_MEM_ERROR:
            return "memory allocation failed";
        case BZ_DATA_ERROR:
            return "data error";
        case BZ_DATA_ERROR_MAGIC:
            return "invalid magic number";
        case BZ_IO_ERROR:
            return "I/O error";
        case BZ_UNEXPECTED_EOF:
            return "unexpected end of file";
        case BZ_OUTBUFF_FULL:
            return "output buffer is full";
        case BZ_CONFIG_ERROR:
            return "configuration error or unsupported platform";
        default:
            return "unknown error";
    }
}

static int read(stream_t *stream, void *dest, size_t size) {
    struct state *state = stream->state;

    bz_stream *bz2strm  = &state->bz2strm;
    bz2strm->next_out   = dest;
    bz2strm->avail_out  = size;

    int status = BZ2_bzDecompress(bz2strm);

    if(status != BZ_OK && status != BZ_STREAM_END) {
        jinue_error("error: bzip2 could not decompress: %s", bzip2_strerror(status));
        return STREAM_ERROR;
    }

    if(bz2strm->avail_out != 0) {
        jinue_error(
                "error: zlib could only decompress %zu of %zu bytes requested",
                size - (size_t)bz2strm->avail_out,
                size
        );
        return STREAM_ERROR;
    }

    return STREAM_SUCCESS;
}

static int initialize_bzip2_stream(bz_stream *bz2strm, const void *addr, size_t size) {
    bz2strm->bzalloc    = NULL;
    bz2strm->bzfree     = NULL;
    bz2strm->opaque     = NULL;
    bz2strm->next_in    = (char *)addr;
    bz2strm->avail_in   = size;
   
    const int verbosity = 0;
    const int small     = 1;
    int status = BZ2_bzDecompressInit(bz2strm, verbosity, small);

    if(status != BZ_OK) {
        return STREAM_ERROR;
    }

    /* Read once with zero bytes of output buffer space available to force the
     * library to read the file header and allocate the memory it needs. */
    bz2strm->next_out   = NULL;
    bz2strm->avail_out  = 0;

    status = BZ2_bzDecompress(bz2strm);

    if(status == BZ_OK) {
        return STREAM_SUCCESS;
    }

    BZ2_bzDecompressEnd(bz2strm);
    return STREAM_ERROR;
}

static void finalize_bzip2_stream(bz_stream *bz2strm) {
    BZ2_bzDecompressEnd(bz2strm);
}

static int reset(stream_t *stream) {
    struct state *oldstate = stream->state;
    struct state *newstate = malloc(sizeof(struct state));

    if(newstate == NULL) {
        return STREAM_ERROR;
    }

    bz_stream *bz2strm  = &newstate->bz2strm;
    int status = initialize_bzip2_stream(bz2strm, oldstate->addr, oldstate->size);

    if(status != STREAM_SUCCESS) {
        jinue_error("error: bzip2 initialization failed (on reset): %s", bzip2_strerror(status));
        free(newstate);
        return STREAM_ERROR;
    }

    newstate->addr  = oldstate->addr;
    newstate->size  = oldstate->size;

    stream->state   = newstate;

    finalize_bzip2_stream(&oldstate->bz2strm);
    free(oldstate);

    return STREAM_SUCCESS;
}

static void finalize(stream_t *stream) {
    struct state *state = stream->state;
    finalize_bzip2_stream(&state->bz2strm);
    free(state);
}

static bool is_header_valid(const void *addr, size_t size) {
    if(size < 4 + sizeof(block_magic)) {
        return false;
    }

    const char *header = addr;

    if(header[0] != 'B' || header[1] != 'Z' || header[2] != 'h') {
        return false;
    }

    if(header[3] < 0x31 || header[3] > 0x39) {
        return false;
    }

    return strncmp(&header[4], block_magic, sizeof(block_magic)) == 0;
}


int bzip2_stream_initialize(stream_t *stream, const void *addr, size_t size) {
    if(!is_header_valid(addr, size)) {
        return STREAM_FORMAT;
    }

    struct state *state = malloc(sizeof(struct state));

    if(state == NULL) {
        return STREAM_ERROR;
    }

    int status = initialize_bzip2_stream(&state->bz2strm, addr, size);

    if(status != STREAM_SUCCESS) {
        jinue_error("error: bzip2 initialization failed: %s", bzip2_strerror(status));
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
