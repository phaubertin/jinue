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
#include <kernel/domain/services/panic.h>
#include <kernel/machine/atomic.h>

/**
 * Initialize a slab cache using parameters from a runtime object type
 *
 * @param cache the cache to initialize
 * @param type the runtime object type definition
 */
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

/**
 * Update object state to reflect an additional descriptor referencing it
 *
 * @param object the object
 * @param desc new descriptor
 */
void open_object(object_header_t *object, const descriptor_t *desc) {
    add_ref_to_object(object);

    if(object->type->open != NULL) {
        object->type->open(object, desc);
    }
}

/**
 * Update object state to reflect a descriptor no longer referencing it
 *
 * @param object the object
 * @param desc descriptor being closed
 */
void close_object(object_header_t *object, const descriptor_t *desc) {
    if(object->type->close != NULL) {
        object->type->close(object, desc);
    }

    sub_ref_to_object(object);
}

/**
 * Mark object as destroyed
 * 
 * The state of the object is destoyed after this function is called but the
 * object is not freed because it might still be referenced.
 *
 * @param object the object
 * @param desc descriptor being closed
 */
void destroy_object(object_header_t *object) {
    if(object_is_destroyed(object)) {
        return;
    }

    mark_object_destroyed(object);

    if(object->type->destroy != NULL) {
        object->type->destroy(object);
    }
}

/**
 * Increment the reference count of an object
 *
 * @param object the object
 */
void add_ref_to_object(object_header_t *object) {
    (void)add_atomic(&object->ref_count, 1);
}

/**
 * Decrement the reference count of an object
 * 
 * If the reference count falls to zero, the object is destroyed and the "free"
 * op from the runtime type definition is called, freeing the object.
 * 
 * This function is called by assembly code. See machine_switch_thread() and
 * machine_switch_and_unref_thread().
 *
 * @param object the object
 */
void sub_ref_to_object(object_header_t *object) {
    int ref_count = add_atomic(&object->ref_count, -1);

    if(ref_count > 0) {
        return;
    }

    if(ref_count != 0) {
        panic("Object reference count decremented to negative value");
    }

    destroy_object(object);

    if(object->type->free != NULL) {
        object->type->free(object);
    }
}
