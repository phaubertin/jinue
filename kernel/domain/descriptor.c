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
#include <kernel/descriptor.h>
#include <kernel/ipc.h>
#include <kernel/object.h>
#include <kernel/process.h>

/**
 * Get an object reference by descriptor in a specified process
 * 
 * @param process process for which the descriptor is looked up
 * @param fd descriptor
 * @return object reference on success, NULL if out of bound
 *
 */
static object_ref_t *dereference_descriptor(process_t *process, int fd) {
    if(fd < 0 || fd > PROCESS_MAX_DESCRIPTORS) {
        return NULL;
    }

    return &process->descriptors[fd];
}

/**
 * Get the object referenced by a descriptor
 * 
 * pobject and pref can each be set to NULL if only one or the other is needed.
 * 
 * @param pobject pointer to where to store the pointer to the object header (out)
 * @param pref pointer to where to store the pointer to the object reference (out)
 * @param process process for which the descriptor is looked up
 * @param fd descriptor
 * @return zero on success, negated error number on error
 *
 */
int dereference_object_descriptor(
        object_header_t **pobject,
        object_ref_t    **pref,
        process_t        *process,
        int               fd) {

    object_ref_t *ref = dereference_descriptor(process, fd);

    if(ref == NULL) {
        return -JINUE_EBADF;
    }

    if(! object_ref_is_in_use(ref)) {
        return -JINUE_EBADF;
    }

    if(object_ref_is_destroyed(ref)) {
        return -JINUE_EIO;
    }

    object_header_t *object = ref->object;

    if(object_is_destroyed(object)) {
        ref->flags |= OBJECT_REF_FLAG_DESTROYED;
        object_close(object, ref);
        return -JINUE_EIO;
    }

    if(pref != NULL) {
        *pref = ref;
    }

    if(pobject != NULL) {
        *pobject = object;
    }

    return 0;
}

/**
 * Get an unused object reference by descriptor
 * 
 * @param pref pointer to where to store the pointer to the object reference (out)
 * @param process process for which the descriptor is looked up
 * @param fd descriptor
 * @return zero on success, negated error number on error
 *
 */
int dereference_unused_descriptor(
        object_ref_t    **pref,
        process_t        *process,
        int               fd) {

    object_ref_t *ref = dereference_descriptor(process, fd);

    if(ref == NULL) {
        return -JINUE_EBADF;
    }

    if(object_ref_is_in_use(ref)) {
        return -JINUE_EBADF;
    }

    *pref = ref;

    return 0;
}

int get_process(process_t **process, int process_fd) {
    object_header_t *object;

    int status = dereference_object_descriptor(&object, NULL, get_current_process(), process_fd);

    if(status < 0) {
        return status;
    }

    if(object->type != object_type_process) {
        return -JINUE_EBADF;
    }

    *process = (process_t *)object;
    return 0;
}
