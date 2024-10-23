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

#include <kernel/domain/alloc/slab.h>
#include <kernel/domain/entities/object.h>

void init_object_cache(slab_cache_t *cache, const object_type_t *type) {
    slab_cache_init(
        cache,
        type->name,
        type->size,
        0,
        type->cache_ctor,
        type->cache_dtor,
        SLAB_DEFAULTS
    );
}

void open_object(object_header_t *object, const descriptor_t *desc) {
    add_ref_to_object(object);

    if(object->type->open != NULL) {
        object->type->open(object, desc);
    }
}

void close_object(object_header_t *object, const descriptor_t *desc) {
    if(object->type->close != NULL) {
        object->type->close(object, desc);
    }

    sub_ref_to_object(object);
}

void destroy_object(object_header_t *object) {
    if(object_is_destroyed(object)) {
        return;
    }

    mark_object_destroyed(object);

    if(object->type->destroy != NULL) {
        object->type->destroy(object);
    }
}

/* This function is called by assembly code. See thread_context_switch_stack(). */
void sub_ref_to_object(object_header_t *object) {
    --object->ref_count;

    if(object->ref_count > 0) {
        return;
    }

    destroy_object(object);

    if(object->type->free != NULL) {
        object->type->free(object);
    }
}
