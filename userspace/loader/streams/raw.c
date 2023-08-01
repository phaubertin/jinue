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

#include <stdlib.h>
#include <string.h>
#include "raw.h"

struct state {
    const char  *addr;
    const char  *start;
    size_t       size;
};

static int read(stream_t *stream, void *dest, size_t size) {
    struct state *state = stream->state;

    size_t remaining = state->start + state->size - state->addr;

    if(size > remaining) {
        return STREAM_ERROR;
    }

    memcpy(dest, state->addr, size);
    state->addr += size;

    return STREAM_SUCCESS;
}

static int reset(stream_t *stream) {
    struct state *state = stream->state;
    state->addr = state->start;
    return STREAM_SUCCESS;
}

static void finalize(stream_t *stream) {
    free(stream->state);
}

int raw_stream_initialize(stream_t *stream, const void *addr, size_t size) {
    struct state *state = malloc(sizeof(struct state));

    if(state == NULL) {
        return STREAM_ERROR;
    }

    state->addr             = addr;
    state->start            = addr;
    state->size             = size;

    stream->state           = state;
    stream->ops.read        = read;
    stream->ops.reset       = reset;
    stream->ops.finalize    = finalize;

    return STREAM_SUCCESS;
}
