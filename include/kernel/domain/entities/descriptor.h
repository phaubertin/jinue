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

#ifndef JINUE_KERNEL_ENTITIES_DESCRIPTOR_H
#define JINUE_KERNEL_ENTITIES_DESCRIPTOR_H

#include <jinue/shared/asm/permissions.h>
#include <kernel/types.h>

/* These flags are numbered downward starting at 31 to not conflict with PERM_... flags
 * which share the same flags field. */

#define DESC_FLAG_NONE          0

#define DESC_FLAG_STATE1        (1<<31)

#define DESC_FLAG_STATE0        (1<<30)

#define DESC_FLAG_OWNER         (1<<29)

#define DESC_FLAG_STATE         (DESC_FLAG_STATE1 | DESC_FLAG_STATE0)

#define DESC_STATE_FREE         0

#define DESC_STATE_RESERVED     DESC_FLAG_STATE0

#define DESC_STATE_OPEN         DESC_FLAG_STATE1

#define DESC_STATE_DESTROYED    (DESC_FLAG_STATE1 | DESC_FLAG_STATE0)


static inline bool descriptor_is_free(const descriptor_t *desc) {
    return (desc->flags & DESC_FLAG_STATE) == DESC_STATE_FREE;
}

static inline bool descriptor_is_reserved(const descriptor_t *desc) {
    return (desc->flags & DESC_FLAG_STATE) == DESC_STATE_RESERVED;
}

static inline bool descriptor_is_open(const descriptor_t *desc) {
    return (desc->flags & DESC_FLAG_STATE) == DESC_STATE_OPEN;
}

static inline bool descriptor_is_destroyed(const descriptor_t *desc) {
    return (desc->flags & DESC_FLAG_STATE) == DESC_STATE_DESTROYED;
}

static inline bool descriptor_is_closeable(const descriptor_t *desc) {
    return descriptor_is_open(desc) || descriptor_is_destroyed(desc);
}

static inline bool descriptor_is_owner(const descriptor_t *desc) {
    return !!(desc->flags & DESC_FLAG_OWNER);
}

static inline bool descriptor_has_permissions(const descriptor_t *desc, int perms) {
    return (desc->flags & perms) == perms;
}

void clear_descriptor(descriptor_t *desc);

int dereference_object_descriptor(
        descriptor_t    *pout,
        process_t       *process,
        int              fd);

void unreference_descriptor_object(descriptor_t *desc);

int reserve_free_descriptor(process_t *process, int fd);

void free_reserved_descriptor(process_t *process, int fd);

void open_descriptor(process_t *process, int fd, const descriptor_t *in);

int close_descriptor(process_t *process, int fd);

ipc_endpoint_t *get_endpoint_from_descriptor(descriptor_t *desc);

process_t *get_process_from_descriptor(descriptor_t *desc);

thread_t *get_thread_from_descriptor(descriptor_t *desc);

#endif
