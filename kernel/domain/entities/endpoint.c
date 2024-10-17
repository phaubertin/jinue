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
#include <kernel/descriptor.h>
#include <kernel/endpoint.h>
#include <kernel/object.h>
#include <kernel/slab.h>
#include <stddef.h>

static void cache_endpoint_ctor(void *buffer, size_t size);

static void open_endpoint(object_header_t *object, const descriptor_t *desc);

static void close_endpoint(object_header_t *object, const descriptor_t *desc);

/** runtime type definition for an IPC endpoint */
static const object_type_t object_type = {
    .all_permissions    = JINUE_PERM_SEND | JINUE_PERM_RECEIVE,
    .name               = "ipc_endpoint",
    .size               = sizeof(ipc_endpoint_t),
    .open               = open_endpoint,
    .close              = close_endpoint,
    .cache_ctor         = cache_endpoint_ctor,
    .cache_dtor         = NULL
};

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
 *
 */
static void cache_endpoint_ctor(void *buffer, size_t size) {
    ipc_endpoint_t *endpoint = buffer;
    
    object_header_init(&endpoint->header, object_type_ipc_endpoint);
    jinue_list_init(&endpoint->send_list);
    jinue_list_init(&endpoint->recv_list);
    endpoint->receivers_count = 0;
}

static void add_receiver(ipc_endpoint_t *endpoint) {
    ++endpoint->receivers_count;
}

static void sub_receiver(ipc_endpoint_t *endpoint) {
    --endpoint->receivers_count;
}

static bool has_receivers(const ipc_endpoint_t *endpoint) {
    return endpoint->receivers_count > 0;
}

static void open_endpoint(object_header_t *object, const descriptor_t *desc) {
    if(descriptor_has_permissions(desc, JINUE_PERM_RECEIVE)) {
        ipc_endpoint_t *endpoint = (ipc_endpoint_t *)object;
        add_receiver(endpoint);
    }
}

static void close_endpoint(object_header_t *object, const descriptor_t *desc) {
    if(descriptor_has_permissions(desc, JINUE_PERM_RECEIVE)) {
        ipc_endpoint_t *endpoint = (ipc_endpoint_t *)object;
        sub_receiver(endpoint);

        if(!has_receivers(endpoint)) {
            object_mark_destroyed(object);
        }
    }
}

/**
 * Perform boot-time initialization for IPC
 *
 */
void initialize_endpoint_cache(void) {
    object_cache_init(&ipc_endpoint_cache, object_type_ipc_endpoint);
}

/**
 * Constructor for IPC endpoint object
 *
 * @return pointer to endpoint on success, NULL on allocation failure
 *
 */
ipc_endpoint_t *construct_endpoint(void) {
    return slab_cache_alloc(&ipc_endpoint_cache);
}
