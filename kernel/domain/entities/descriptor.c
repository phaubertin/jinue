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
 * A cleared descriptor is in the free state (i.e. is unused) and is not
 * referring to any object.
 * 
 * @param desc descriptor
 */
void descriptor_clear(descriptor_t *desc) {
    desc->flags     = DESC_STATE_FREE;
    desc->object    = NULL;
    desc->cookie    = 0;
}

/**
 * Get an object reference by descriptor number in a specified process
 * 
 * This function does not do any kind of locking or reference count update, so
 * it should only be called in this file by functions that do these things.
 * 
 * @param process process for which the descriptor is looked up
 * @param fd descriptor number (DESC_STATE_... constant)
 * @return object reference on success, NULL if out of bound
 */
static descriptor_t *dereference(process_t *process, int fd) {
    if(fd < 0 || fd > JINUE_DESC_NUM) {
        return NULL;
    }

    return &process->descriptors[fd];
}

/**
 * Set the state of a descriptor.
 * 
 * This function does not do any kind of locking, so it should only be called
 * in this file by functions that it when needed.
 * 
 * @param desc descriptor in a process' descriptor array
 * @param state state to set (DESC_STATE_... constant)
 * @return object reference on success, NULL if out of bound
 */
static void set_state(descriptor_t *desc, int state)  {
    desc->flags &= ~DESC_FLAG_STATE;
    desc->flags |= state;
}

/**
 * Portion of descriptor_access_object() performed under lock
 * 
 * @param pout pointer to where to copy the descriptor (out)
 * @param process process for which the descriptor is looked up
 * @param desc referenced descriptor
 * @return zero on success, negated error number on error
 */
static int descriptor_access_object_locked(
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
        set_state(desc, DESC_STATE_DESTROYED);
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
 * This function finds a descriptor by descriptor number is a specified
 * process. It expects the descriptor to reference an object (i.e. be in the
 * open state) and fails with JINUE_EBADF if it doesn't, unless the descriptor
 * is in the destroyed state, in which case it fails with JINUE_EIO.
 * 
 * It also checks that the object referenced hasn't been destroyed. If it has,
 * it changes the descriptor's state to destroyed, decrement the referenced
 * object's reference count (because the descriptor no longer references it)
 * and fails with JINUE_EIO.
 * 
 * If the call succeeds, it copies the descriptor to the storage pointed to by
 * the pout argument and increments the reference count of the referenced
 * object. These two things ensure the caller can continue to access the
 * descriptor data and referenced object even while not under the process'
 * descriptor lock and even if the descriptor is concurrently modified.
 * 
 * The caller *must* decrement the referenced object reference call when it is
 * done with it to free the reference added by this call. This can be done by
 * calling descriptor_unreference_object() on the descriptor copy.
 * 
 * @param pout pointer to where to copy the descriptor (out)
 * @param process process in which the descriptor is looked up
 * @param fd descriptor number
 * @return zero on success, negated error number on error
 */
int descriptor_access_object(
        descriptor_t    *pout,
        process_t       *process,
        int              fd) {

    descriptor_t *desc = dereference(process, fd);

    if(desc == NULL) {
        return -JINUE_EBADF;
    }

    spin_lock(&process->descriptors_lock);

    int status = descriptor_access_object_locked(pout, process, desc);
    
    spin_unlock(&process->descriptors_lock);

    return status;
}

/**
 * Unreference the object referenced by a descriptor
 * 
 * Must be called on the copy of the descriptor obtained by calling
 * descriptor_access_object() when the caller is done accessing the
 * object.
 * 
 * @param desc descriptor
 */
void descriptor_unreference_object(descriptor_t *desc) {
    sub_ref_to_object(desc->object);
}

/**
 * Reserve an unused descriptor by descriptor number
 * 
 * This function ensures the descriptor is free. If it is, it sets it state to
 * reserve (DESC_STATE_RESERVED) to prevent concurrent attempts to assign it.
 * 
 * Once a descriptor is reserved, it must be set with descriptor_open(), or the
 * reservation must be released with descriptor_free_reservation() if rolling back
 * becomes necessary.
 * 
 * This two step process allows the caller to confirm the availability of the
 * descriptor before it starts allocating resources or performing some
 * similarly expensive operation, and it ensures this operation does not need
 * to be done while holding the process' descriptor lock.
 * 
 * @param pdesc pointer to where to store the pointer to the object reference (out)
 * @param process process for which the descriptor is looked up
 * @param fd descriptor number
 * @return zero on success, negated error number on error
 */
int descriptor_reserve_unused(process_t *process, int fd) {
    descriptor_t *desc = dereference(process, fd);

    if(desc == NULL) {
        return -JINUE_EBADF;
    }

    spin_lock(&process->descriptors_lock);

    if(!descriptor_is_free(desc)) {
        spin_unlock(&process->descriptors_lock);
        return -JINUE_EBADF;
    }

    set_state(desc, DESC_STATE_RESERVED);

    spin_unlock(&process->descriptors_lock);

    return 0;
}

/**
 * Free a reserved descriptor
 * 
 * This function must be called for a descriptor reserved with
 * descriptor_reserve_unused() if the descriptor will not be set with
 * descriptor_open().
 * 
 * @param process process for which the descriptor is looked up
 * @param fd descriptor number
 */
void descriptor_free_reservation(process_t *process, int fd) {
    descriptor_t *desc = dereference(process, fd);

    spin_lock(&process->descriptors_lock);

    assert(descriptor_is_reserved(desc));

    descriptor_clear(desc);

    spin_unlock(&process->descriptors_lock);
}

/**
 * Open a descriptor
 * 
 * This function assigns an object to a descriptor and sets its flags and
 * cookie. The object's reference count is incremented to reflect the new
 * reference by the descriptor. The object's open op is also called.
 * 
 * The descriptor must have been reserved with descriptor_reserve_unused() before
 * calling this function. This function does not do error checking because this
 * will have been done by descriptor_reserve_unused().
 * 
 * @param process process in which the descriptor is opened
 * @param fd descriptor number
 * @param in data to set on the descriptor
 */
void descriptor_open(process_t *process, int fd, const descriptor_t *in) {
    descriptor_t *desc = dereference(process, fd);

    spin_lock(&process->descriptors_lock);

    assert(descriptor_is_reserved(desc));

    *desc = *in;
    
    set_state(desc, DESC_STATE_OPEN);

    spin_unlock(&process->descriptors_lock);

    open_object(in->object, in);
}

/**
 * Close a descriptor
 * 
 * This function closes a descriptor so it becomes available for reuse. In
 * addition to clearing the descriptor state, it also calls the referenced
 * object's close op if the descriptor is initially in the open state.
 * 
 * @param process process in which the descriptor is closed
 * @param fd descriptor number
 * @return zero on success, negated error number on error
 */
int descriptor_close(process_t *process, int fd) {
    descriptor_t *desc = dereference(process, fd);

    if(desc == NULL) {
        return -JINUE_EBADF;
    }

    spin_lock(&process->descriptors_lock);

    if(! descriptor_is_closeable(desc)) {
        spin_unlock(&process->descriptors_lock);
        return -JINUE_EBADF;
    }

    const descriptor_t copy = *desc;

    descriptor_clear(desc);
    
    spin_unlock(&process->descriptors_lock);

    if(descriptor_is_open(&copy)) {
        close_object(copy.object, &copy);
    }

    return 0;
}

/**
 * Get IPC endpoint referenced by descriptor
 * 
 * If the specified descriptor refers to an IPC endpoint, a pointer to that
 * endpoint is returned. Otherwise, the function fails by returning NULL.
 * 
 * This function is typically called on a descriptor copy obtain by calling
 * descriptor_access_object().
 * 
 * @param desc descriptor
 * @return IPC endpoint on success, NULL on failure
 */
ipc_endpoint_t *descriptor_get_endpoint(descriptor_t *desc) {
    object_header_t *object = desc->object;

    if(object->type != object_type_ipc_endpoint) {
        return NULL;
    }

    return (ipc_endpoint_t *)object;
}

/**
 * Get process referenced by descriptor
 * 
 * If the specified descriptor refers to a process, a pointer to that process
 * is returned. Otherwise, the function fails by returning NULL.
 * 
 * This function is typically called on a descriptor copy obtain by calling
 * descriptor_access_object().
 * 
 * @param desc descriptor
 * @return process on success, NULL on failure
 */
process_t *descriptor_get_process(descriptor_t *desc) {
    object_header_t *object = desc->object;

    if(object->type != object_type_process) {
        return NULL;
    }

    return (process_t *)object;
}

/**
 * Get thread referenced by descriptor
 * 
 * If the specified descriptor refers to a thread, a pointer to that thread
 * is returned. Otherwise, the function fails by returning NULL.
 * 
 * This function is typically called on a descriptor copy obtain by calling
 * descriptor_access_object().
 * 
 * @param desc descriptor
 * @return thread on success, NULL on failure
 */
thread_t *descriptor_get_thread(descriptor_t *desc) {
    object_header_t *object = desc->object;

    if(object->type != object_type_thread) {
        return NULL;
    }

    return (thread_t *)object;
}
