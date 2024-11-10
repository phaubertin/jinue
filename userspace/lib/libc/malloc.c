/*
 * Copyright (C) 2023-2024 Philippe Aubertin.
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

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include "brk.h"
#include "malloc.h"

/* minimum allocation size, in bytes */
#define MIN_SBRK_SIZE   (16 * 1024)

/* cleared: allocatable buffer (allocated or free)
 * set: start or end terminator of a "frame" of contiguous buffers */
#define FLAG_TERMINATOR (1<<0)

/* only meaningful when FLAG_TERMINATOR is cleared
 *
 * cleared: buffer is allocated
 * set: buffer is free */
#define FLAG_FREE       (1<<1)

/* only meaningful when FLAG_TERMINATOR is set
 *
 * cleared: terminator marks the start of a frame
 * set: terminator marks the end of a frame */
#define FLAG_END        FLAG_FREE

#define FLAGS_MASK      (FLAG_TERMINATOR | FLAG_FREE)

struct header {
    /* previous header pointer, flags in two least significant bits */
    uintptr_t prev_and_flags;

    /* buffer: size of buffer excluding header, next header is right after buffer
     * end terminator: pointer to start terminator of next frame
     * start terminator: unused, next header is right after current header */
    uintptr_t size_or_next;
};

static struct header *heap_head = NULL;

static struct header *heap_tail = NULL;

void *malloc(size_t size) {
    return __malloc_perrno(size, &errno);
}

static bool is_terminator(const struct header *header) {
    return !!(header->prev_and_flags & FLAG_TERMINATOR);
}

static bool is_free(const struct header *header) {
    return (header->prev_and_flags & FLAGS_MASK) == FLAG_FREE;
}

static bool is_start(const struct header *header) {
    return (header->prev_and_flags & FLAGS_MASK) == FLAG_TERMINATOR;
}

static bool is_end(const struct header *header) {
    return (header->prev_and_flags & FLAGS_MASK) == (FLAG_TERMINATOR | FLAG_END);
}

static size_t bufsize(const struct header *header) {
    if(is_terminator(header)) {
        return 0;
    }
    return (size_t)header->size_or_next;
}

static struct header *prev(const struct header *header) {
    return (void *)(header->prev_and_flags & ~(uintptr_t)FLAGS_MASK);
}

static struct header *next(const struct header *header) {
    if(is_end(header)) {
        return (void *)header->size_or_next;
    }
    if(is_start(header)) {
        return (void *)((uintptr_t)header + sizeof(struct header));
    }
    return (void *)((uintptr_t)header + sizeof(struct header) + bufsize(header));
}

static void *allocate_from_buffer(struct header *buffer, size_t size) {
    size_t old_bufsize = bufsize(buffer);

    if(old_bufsize >= size + 2 * sizeof(struct header)) {
        buffer->size_or_next        = size;

        struct header *new_buffer   = next(buffer);
        new_buffer->prev_and_flags  = (uintptr_t)buffer | FLAG_FREE;
        new_buffer->size_or_next    = old_bufsize - size - sizeof(struct header);
    }

    buffer->prev_and_flags &= ~(uintptr_t)FLAG_FREE;
    return buffer + 1;
}

#define FAILED_SBRK ((void *)-1)

static void *sbrk_else_null(intptr_t incr, int *perrno) {
    void *old_break = __sbrk_perrno(incr, perrno);
    return (old_break == (void *)-1) ? NULL : old_break;
}

static void *align_break(int *perrno) {
    void *current_break = sbrk_else_null(0, perrno);

    if(current_break == NULL) {
        return NULL;
    }

    uintptr_t alignment = (uintptr_t)current_break & (sizeof(struct header) - 1);

    if(alignment == 0) {
        return current_break;
    }

    void *ret = sbrk_else_null(sizeof(struct header) - alignment, perrno);

    if(ret == NULL) {
        return NULL;
    }

    return sbrk_else_null(0, perrno);
}

static void *allocate_with_sbrk(size_t size, int *perrno) {
    void *current_break = align_break(perrno);

    if(current_break == NULL) {
        return NULL;
    }

    size_t alloc_size;

    if(size < MIN_SBRK_SIZE) {
        alloc_size = MIN_SBRK_SIZE;
    }
    else {
        /* align to header size */
        alloc_size = (size + sizeof(struct header) - 1) & ~(sizeof(struct header) - 1);
    }

    struct header *buffer;

    if(heap_tail == NULL || (uintptr_t)heap_tail + sizeof(struct header) != (uintptr_t)current_break) {
        /* Common case: we need to allocate a new frame.
         *
         * Three headers: the one for the new buffer and two terminators for the
         * new frame */
        void *ret = sbrk_else_null(alloc_size + 3 * sizeof(struct header), perrno);

        if(ret == NULL) {
            return NULL;
        }

        struct header *start    = (struct header *)current_break;
        start->prev_and_flags   = (uintptr_t)heap_tail | FLAG_TERMINATOR;
        start->size_or_next     = 0;

        buffer                  = start + 1;
        buffer->prev_and_flags  = (uintptr_t)start | FLAG_FREE;
        buffer->size_or_next    = alloc_size;

        if(heap_head == NULL) {
            heap_head = start;
        }
    }
    else {
        /* The newly allocated memory will be allocated just after the end of
         * the existing last frame. This means we don't need a new frame, we
         * just need to extend the existing one. */
        struct header *tail_prev = prev(heap_tail);

        if(is_free(tail_prev)) {
            /* Not only can we extend the existing frame, but the last buffer
             * in that frame is free, so we can just extend that buffer instead
             * of creating a new one. */
            buffer = tail_prev;

            /* only one header size for the new end terminator */
            void *ret = sbrk_else_null(alloc_size - bufsize(buffer), perrno);

            if(ret == NULL) {
                return NULL;
            }

            buffer->size_or_next = alloc_size;
        }
        else {
            /* We need a new buffer but we can extend the existing frame and
             * put it there. */
            buffer = heap_tail;

            void *ret = sbrk_else_null(alloc_size + sizeof(struct header), perrno);

            if(ret == NULL) {
                return NULL;
            }

            buffer->prev_and_flags &= ~(uintptr_t)FLAGS_MASK;
            buffer->prev_and_flags |= FLAG_FREE;
            buffer->size_or_next = alloc_size;
        }
    }

    struct header *end      = next(buffer);
    end->prev_and_flags     = (uintptr_t)buffer | FLAG_TERMINATOR | FLAG_END;
    end->size_or_next       = NULL;
    heap_tail               = end;

    return allocate_from_buffer(buffer, size);
}

void *__malloc_perrno(size_t size, int *perrno) {
    if(size == 0) {
        return NULL;
    }

    /* Corner case to keep in mind: heap_head is initially NULL. */
    for(struct header *header = heap_head; header != NULL; header = next(header)) {
        /* first fit
         *
         * is_free() returns false for terminators so we don't have to handle
         * terminators specially here. */
        if(is_free(header) && bufsize(header) >= size) {
            return allocate_from_buffer(header, size);
        }
    }

    /* Could not find a free buffer to satisfy allocation, let's call sbrk(). */
    return allocate_with_sbrk(size, perrno);
}

void free(void *ptr) {
    if(ptr == NULL) {
        return;
    }

    struct header *buffer       = (struct header *)ptr - 1;
    struct header *next_header  = next(buffer);

    if(is_free(next_header)) {
        buffer->size_or_next += (next_header->size_or_next + sizeof(struct header));
    }

    struct header *prev_header = prev(buffer);

    if(is_free(prev_header)) {
        prev_header->size_or_next += (buffer->size_or_next + sizeof(struct header));
        buffer = prev_header;
    }

    buffer->prev_and_flags |= FLAG_FREE;
}
