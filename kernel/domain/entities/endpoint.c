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

#include <jinue/shared/asm/errno.h>
#include <jinue/shared/asm/permissions.h>
#include <kernel/domain/alloc/slab.h>
#include <kernel/domain/entities/descriptor.h>
#include <kernel/domain/entities/endpoint.h>
#include <kernel/domain/entities/object.h>
#include <kernel/domain/services/ipc.h>
#include <kernel/machine/atomic.h>
#include <kernel/machine/spinlock.h>
#include <kernel/utils/list.h>
#include <stddef.h>

static void cache_ctor_op(void *buffer, size_t size);

static void open_op(object_header_t *object, const descriptor_t *desc);

static void close_op(object_header_t *object, const descriptor_t *desc);

static void destroy_op(object_header_t *object);

static void free_op(object_header_t *object);

static const object_type_t object_type = {
    .all_permissions    = JINUE_PERM_SEND | JINUE_PERM_RECEIVE,
    .name               = "ipc_endpoint",
    .size               = sizeof(ipc_endpoint_t),
    .open               = open_op,
    .close              = close_op,
    .destroy            = destroy_op,
    .free               = free_op,
    .cache_ctor         = cache_ctor_op,
    .cache_dtor         = NULL
};

/** runtime type definition for an IPC endpoint */
const object_type_t *object_type_ipc_endpoint = &object_type;

/** slab cache used for allocating IPC endpoint objects */
static slab_cache_t ipc_endpoint_cache;

/**
 * Object constructor for IPC endpoint slab allocator
 *
 * All currently recognized flags will be deprecated.
 *
 * @param buffer IPC endpoint object being constructed
 * @param size size in bytes of the IPC endpoint object (ignored)
 */
static void cache_ctor_op(void *buffer, size_t size) {
    ipc_endpoint_t *endpoint = buffer;
    
    init_object_header(&endpoint->header, object_type_ipc_endpoint);
    init_list(&endpoint->send_list);
    init_list(&endpoint->recv_list);
    init_spinlock(&endpoint->lock);
    endpoint->receivers_count = 0;
}

/**
 * Add a reference that can be used to receive on the endpoint
 *
 * @param endpoint the endpoint
 */
static void add_receiver(ipc_endpoint_t *endpoint) {
    add_atomic(&endpoint->receivers_count, 1);
}

/**
 * Remove a reference that can be used to receive on the endpoint
 *
 * @param endpoint the endpoint
 * @return updated number of references allowed to receive
 */
static int sub_receiver(ipc_endpoint_t *endpoint) {
    return add_atomic(&endpoint->receivers_count, -1);
}

/**
 * Open an IPC endpoint
 * 
 * This function is defined as the "open" op in the runtime type definition,
 * called when a new descriptor references the endpoint.
 *
 * @param object the endpoint object
 * @param desc the new descriptor
 */
static void open_op(object_header_t *object, const descriptor_t *desc) {
    if(descriptor_has_permissions(desc, JINUE_PERM_RECEIVE)) {
        ipc_endpoint_t *endpoint = (ipc_endpoint_t *)object;
        add_receiver(endpoint);
    }
}

/**
 * Close an IPC endpoint
 * 
 * This function is defined as the "close" op in the runtime type definition,
 * called when a a descriptor that references the endpoint is closed and stops
 * referencing it.
 *
 * @param object the endpoint object
 * @param desc the descriptor being closed
 */
static void close_op(object_header_t *object, const descriptor_t *desc) {
    if(descriptor_has_permissions(desc, JINUE_PERM_RECEIVE)) {
        ipc_endpoint_t *endpoint = (ipc_endpoint_t *)object;
        int receivers = sub_receiver(endpoint);

        if(receivers < 1) {
            destroy_object(object);
        }
    }
}

/**
 * Initialize the IPC endpoint slab cache
 */
void initialize_endpoint_cache(void) {
    init_object_cache(&ipc_endpoint_cache, object_type_ipc_endpoint);
}

/**
 * Constructor for IPC endpoint object
 *
 * @return endpoint on success, NULL on allocation failure
 */
ipc_endpoint_t *endpoint_new(void) {
    return slab_cache_alloc(&ipc_endpoint_cache);
}

/**
 * Destroy an IPC endpoint
 * 
 * This function is defined as the "destroy" op in the runtime type definition.
 *
 * @param object the endpoint object
 */
static void destroy_op(object_header_t *object) {
    ipc_endpoint_t *endpoint = (ipc_endpoint_t *)object;

    while(true) {
        thread_t *sender = list_dequeue(&endpoint->send_list, thread_t, thread_list);
        
        if(sender == NULL) {
            break;
        }

        abort_message(sender);
    }

    while(true) {
        thread_t *receiver = list_dequeue(&endpoint->recv_list, thread_t, thread_list);
        
        if(receiver == NULL) {
            break;
        }

        abort_message(receiver);
    }
}

/**
 * Free an IPC endpoint
 * 
 * This function is defined as the "free" op in the runtime type definition,
 * called automatically when the endpoint's reference count falls to zero.
 *
 * @param object the endpoint object
 */
static void free_op(object_header_t *object) {
    slab_cache_free(object);
}
