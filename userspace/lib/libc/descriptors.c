/*
 * Copyright (C) 2024 Philippe Aubertin.
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

#include <jinue/jinue.h>
#include <errno.h>
#include <internals.h>
#include "descriptors.h"
#include "malloc.h"

struct fdlink {
    struct fdlink   *next;
    int              fd;
};

static struct {
    int              next;
    struct fdlink   *freelist;
    struct fdlink   *freelinks;
} alloc_state = {
    .next       = JINUE_DESC_LAST + 1,
    .freelist   = NULL,
    .freelinks  = NULL
};

int __allocate_descriptor(void) {
    return __allocate_descriptor_perrno(&errno);
}

static void append(struct fdlink **root, struct fdlink *link) {
    link->next  = *root;
    *root       = link;
}

static struct fdlink *get(struct fdlink **root) {
    struct fdlink *link = *root;

    if(link != NULL) {
        *root = link->next;
    }

    return link;
}

static int allocate_new_descriptor(int *perrno) {
    if(alloc_state.next >= JINUE_DESC_NUM) {
        *perrno = EAGAIN;
        return -1;
    }

    /* We don't need the link now, only when the descriptor is freed. Let's allocate it now
     * anyway because we want this function to fail if we can't allocate, we don't want the
     * failure to happen when we free the descriptor. */
    struct fdlink *link = __malloc_perrno(sizeof(struct fdlink), perrno);

    if(link == NULL) {
        return -1;
    }

    append(&alloc_state.freelinks, link);

    return (alloc_state.next)++;
}

int __allocate_descriptor_perrno(int *perrno) {
    /* Fast allocation path: reuse a descriptor number previously freed by free_descriptor(). */

    struct fdlink *link = get(&alloc_state.freelist);

    if(link != NULL) {
        append(&alloc_state.freelinks, link);
        return link->fd;
    }

    /* Slow allocation path: allocate a new, never used descriptor. */
    
    return allocate_new_descriptor(perrno);
}

void __free_descriptor(int fd) {
    struct fdlink *link = get(&alloc_state.freelinks);
    link->fd = fd;
    append(&alloc_state.freelist, link);
}
