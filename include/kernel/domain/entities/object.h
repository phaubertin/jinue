/*
 * Copyright (C) 2019-2024 Philippe Aubertin.
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

#ifndef JINUE_KERNEL_ENTITIES_OBJECT_H
#define JINUE_KERNEL_ENTITIES_OBJECT_H

#include <jinue/shared/asm/permissions.h>
#include <kernel/types.h>


#define OBJECT_FLAG_NONE            0

#define OBJECT_FLAG_DESTROYED       (1<<0)


static inline void mark_object_destroyed(object_header_t *object) {
    object->flags |= OBJECT_FLAG_DESTROYED;
}

static inline bool object_is_destroyed(object_header_t *object) {
    return !!(object->flags & OBJECT_FLAG_DESTROYED);
}

static inline void init_object_header(object_header_t *object, const object_type_t *type) {
    object->type = type;
    object->ref_count   = 0;
    object->flags       = OBJECT_FLAG_NONE;
}

static inline void add_ref_to_object(object_header_t *object) {
    ++object->ref_count;
}

static inline void sub_ref_to_object(object_header_t *object) {
    /** TODO free at zero */
    --object->ref_count;
}

void init_object_cache(slab_cache_t *cache, const object_type_t *type);

void open_object(object_header_t *object, const descriptor_t *desc);

void close_object(object_header_t *object, const descriptor_t *desc);

#endif
