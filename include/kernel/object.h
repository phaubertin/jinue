/*
 * Copyright (C) 2019 Philippe Aubertin.
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

#ifndef JINUE_KERNEL_OBJECT_H
#define JINUE_KERNEL_OBJECT_H

#include <kernel/types.h>

/* flag bits 0..7 are common flags, flag bits 8 and up are per object type flags */

#define OBJECT_FLAG_NONE            0

#define OBJECT_FLAG_DESTROYED       (1<<0)


#define OBJECT_REF_FLAG_NONE        0

#define OBJECT_REF_FLAG_IN_USE      (1<<0)

#define OBJECT_REF_FLAG_DESTROYED   (1<<1)

#define OBJECT_REF_FLAG_OWNER       (1<<2)


#define OBJECT_TYPE_THREAD          1

#define OBJECT_TYPE_IPC             2

#define OBJECT_TYPE_PROCESS         3


static inline void object_mark_destroyed(object_header_t *header) {
    header->flags |= OBJECT_FLAG_DESTROYED;
}

static inline bool object_is_destroyed(object_header_t *header) {
    return !!(header->flags & OBJECT_FLAG_DESTROYED);
}

static inline bool object_ref_is_in_use(object_ref_t *ref) {
    return ref != NULL && (ref->flags & OBJECT_REF_FLAG_IN_USE);
}

static inline bool object_ref_is_destroyed(object_ref_t *ref) {
    return !!(ref->flags & OBJECT_REF_FLAG_DESTROYED);
}

static inline bool object_ref_is_owner(object_ref_t *ref) {
    return !!(ref->flags & OBJECT_REF_FLAG_OWNER);
}

static inline void object_header_init(object_header_t *header, int type) {
    header->type        = type;
    header->ref_count   = 0;
    header->flags       = OBJECT_FLAG_NONE;
}

static inline void object_addref(object_header_t *header) {
    ++header->ref_count;
}

static inline void object_subref(object_header_t *header) {
    /** TODO free at zero */
    --header->ref_count;
}

#endif
