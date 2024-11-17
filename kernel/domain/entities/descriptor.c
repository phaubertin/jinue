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
#include <kernel/machine/spinlock.h>
#include <assert.h>


/**
 * Clear a descriptor
 * 
 * A cleared descriptor is unused and is not referring to any object.
 * 
 * @param desc descriptor
 *
 */
void clear_descriptor(descriptor_t *desc) {
    desc->flags     = DESCRIPTOR_STATE_FREE;
    desc->object    = NULL;
    desc->cookie    = 0;
}

/**
 * Get an object reference by descriptor in a specified process
 * 
 * @param process process for which the descriptor is looked up
 * @param fd descriptor number
 * @return object reference on success, NULL if out of bound
 *
 */
static descriptor_t *dereference_descriptor(process_t *process, int fd) {
    if(fd < 0 || fd > JINUE_DESC_NUM) {
        return NULL;
    }

    return &process->descriptors[fd];
}

static void set_state(descriptor_t *desc, int state)  {
    desc->flags &= ~DESCRIPTOR_FLAG_STATE;
    desc->flags |= state;
}

/**
 * Portion of dereference_object_descriptor() performed under locked
 * 
 * @param pout pointer to where to copy the descriptor (out)
 * @param process process for which the descriptor is looked up
 * @param desc referenced descriptor
 * @return zero on success, negated error number on error
 *
 */
static int dereference_object_descriptor_locked(
        descriptor_t    *pout,
        process_t       *process,
        descriptor_t    *desc) {
    
    if(descriptor_is_destroyed(desc)) {
        return -JINUE_EIO;
    }
    
    if(! descriptor_is_open(desc)) {
        return -JINUE_EBADF;
    }

    object_header_t *object = desc->object;

    if(object_is_destroyed(object)) {
        set_state(desc, DESCRIPTOR_STATE_DESTROYED);
        close_object(object, desc);
        return -JINUE_EIO;
    }

    add_ref_to_object(desc->object);
    *pout = *desc;

    return 0;
}

/**
 * Dereference an object descriptor
 * 
 * Checked that the descriptor actually references an object and that it hasn't
 * been destroyed. Adds a reference to the object on success.
 * 
 * @param pout pointer to where to copy the descriptor (out)
 * @param process process for which the descriptor is looked up
 * @param fd descriptor number
 * @return zero on success, negated error number on error
 *
 */
int dereference_object_descriptor(
        descriptor_t    *pout,
        process_t       *process,
        int              fd) {

    descriptor_t *desc = dereference_descriptor(process, fd);

    if(desc == NULL) {
        return -JINUE_EBADF;
    }

    spin_lock(&process->descriptors_lock);

    int status = dereference_object_descriptor_locked(pout, process, desc);
    
    spin_unlock(&process->descriptors_lock);

    return status;
}

/**
 * Unreference the object referenced by a descriptor
 * 
 * Should be called after dereference_object_descriptor() when you are done
 * accessing the object
 * 
 * @param desc descriptor
 *
 */
void unreference_descriptor_object(descriptor_t *desc) {
    sub_ref_to_object(desc->object);
}

/**
 * Reserve an unused descriptor by descriptor number
 * 
 * @param pdesc pointer to where to store the pointer to the object reference (out)
 * @param process process for which the descriptor is looked up
 * @param fd descriptor number
 * @return zero on success, negated error number on error
 *
 */
int reserve_unused_descriptor(process_t *process, int fd) {
    descriptor_t *desc = dereference_descriptor(process, fd);

    if(desc == NULL) {
        return -JINUE_EBADF;
    }

    spin_lock(&process->descriptors_lock);

    if(!descriptor_is_free(desc)) {
        spin_unlock(&process->descriptors_lock);
        return -JINUE_EBADF;
    }

    set_state(desc, DESCRIPTOR_STATE_RESERVED);

    spin_unlock(&process->descriptors_lock);

    return 0;
}

void free_descriptor(process_t *process, int fd) {
    descriptor_t *desc = dereference_descriptor(process, fd);

    spin_lock(&process->descriptors_lock);

    assert(descriptor_is_reserved(desc));

    clear_descriptor(desc);

    spin_unlock(&process->descriptors_lock);
}

void open_descriptor(process_t *process, int fd, const descriptor_t *in) {
    descriptor_t *desc = dereference_descriptor(process, fd);

    spin_lock(&process->descriptors_lock);

    assert(descriptor_is_reserved(desc));

    *desc = *in;
    
    set_state(desc, DESCRIPTOR_STATE_OPEN);

    spin_unlock(&process->descriptors_lock);

    open_object(in->object, in);
}

int close_descriptor(process_t *process, int fd) {
    descriptor_t *desc = dereference_descriptor(process, fd);

    if(desc == NULL) {
        return -JINUE_EBADF;
    }

    spin_lock(&process->descriptors_lock);

    if(! descriptor_is_closeable(desc)) {
        spin_unlock(&process->descriptors_lock);
        return -JINUE_EBADF;
    }

    const descriptor_t copy = *desc;

    clear_descriptor(desc);
    
    spin_unlock(&process->descriptors_lock);

    if(descriptor_is_open(&copy)) {
        close_object(copy.object, &copy);
    }

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
