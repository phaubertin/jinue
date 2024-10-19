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

#include <jinue/shared/asm/errno.h>
#include <kernel/domain/entities/descriptor.h>
#include <kernel/domain/entities/endpoint.h>
#include <kernel/domain/entities/object.h>
#include <kernel/domain/entities/process.h>
#include <kernel/domain/entities/thread.h>

/**
 * Get an object reference by descriptor in a specified process
 * 
 * @param process process for which the descriptor is looked up
 * @param fd descriptor
 * @return object reference on success, NULL if out of bound
 *
 */
static descriptor_t *dereference_descriptor(process_t *process, int fd) {
    if(fd < 0 || fd > PROCESS_MAX_DESCRIPTORS) {
        return NULL;
    }

    return &process->descriptors[fd];
}

/**
 * Get the object referenced by a descriptor
 * 
 * @param pdesc pointer to where to store the pointer to the object reference (out)
 * @param process process for which the descriptor is looked up
 * @param fd descriptor
 * @return zero on success, negated error number on error
 *
 */
int dereference_object_descriptor(
        descriptor_t    **pdesc,
        process_t        *process,
        int               fd) {

    descriptor_t *desc = dereference_descriptor(process, fd);

    if(desc == NULL) {
        return -JINUE_EBADF;
    }

    if(! descriptor_is_in_use(desc)) {
        return -JINUE_EBADF;
    }

    if(descriptor_is_destroyed(desc)) {
        return -JINUE_EIO;
    }

    object_header_t *object = desc->object;

    if(object_is_destroyed(object)) {
        desc->flags |= DESCRIPTOR_FLAG_DESTROYED;
        close_object(object, desc);
        return -JINUE_EIO;
    }

    *pdesc = desc;

    return 0;
}

/**
 * Get an unused object reference by descriptor
 * 
 * @param pdesc pointer to where to store the pointer to the object reference (out)
 * @param process process for which the descriptor is looked up
 * @param fd descriptor
 * @return zero on success, negated error number on error
 *
 */
int dereference_unused_descriptor(
        descriptor_t    **pdesc,
        process_t        *process,
        int               fd) {

    descriptor_t *desc = dereference_descriptor(process, fd);

    if(desc == NULL) {
        return -JINUE_EBADF;
    }

    if(descriptor_is_in_use(desc)) {
        return -JINUE_EBADF;
    }

    *pdesc = desc;

    return 0;
}

ipc_endpoint_t *get_endpoint_from_descriptor(descriptor_t *desc) {
    object_header_t *object = desc->object;

    if(object->type != object_type_ipc_endpoint) {
        return NULL;
    }

    return (ipc_endpoint_t *)object;
}

process_t *get_process_from_descriptor(descriptor_t *desc) {
    object_header_t *object = desc->object;

    if(object->type != object_type_process) {
        return NULL;
    }

    return (process_t *)object;
}

thread_t *get_thread_from_descriptor(descriptor_t *desc) {
    object_header_t *object = desc->object;

    if(object->type != object_type_thread) {
        return NULL;
    }

    return (thread_t *)object;
}
