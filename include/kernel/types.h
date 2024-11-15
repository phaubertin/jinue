/*
 * Copyright (C) 2019 Philippe Aubertin.
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

#ifndef JINUE_KERNEL_TYPES_H
#define JINUE_KERNEL_TYPES_H

#include <jinue/shared/asm/ipc.h>
#include <jinue/shared/asm/descriptors.h>
#include <jinue/shared/types.h>
#include <kernel/machine/types.h>
#include <kernel/utils/list.h>
#include <kernel/typedeps.h>

struct boot_heap_pushed_state {
    struct boot_heap_pushed_state   *next;
};

typedef struct {
    void                            *heap_ptr;
    struct boot_heap_pushed_state   *heap_pushed_state;
    void                            *current_page;
    void                            *page_limit;
    void                            *first_page_at_16mb;
} boot_alloc_t;

typedef struct slab_cache_t slab_cache_t;

typedef void (*slab_ctor_t)(void *, size_t);

typedef struct descriptor_t descriptor_t;

typedef struct object_header_t object_header_t;

typedef void (*descriptor_func_t)(object_header_t *, const descriptor_t *);

typedef void (*object_func_t)(object_header_t *);

typedef struct {
    int                  all_permissions;
    char                *name;
    size_t               size;
    descriptor_func_t    open;
    descriptor_func_t    close;
    object_func_t        destroy;
    object_func_t        free;
    slab_ctor_t          cache_ctor;
    slab_ctor_t          cache_dtor;
} object_type_t;

struct object_header_t {
    const object_type_t *type;
    int                  ref_count;
    int                  flags;
};

struct descriptor_t {
    object_header_t *object;
    uintptr_t        flags;
    uintptr_t        cookie;
};

typedef struct {
    object_header_t header;
    addr_space_t    addr_space;
    int             running_threads_count;
    descriptor_t    descriptors[JINUE_DESC_NUM];
} process_t;

typedef enum {
    THREAD_STATE_ZOMBIE,
    THREAD_STATE_READY,
    THREAD_STATE_RUNNING,
    THREAD_STATE_BLOCKED
} thread_state_t;

struct thread_t {
    object_header_t          header;
    machine_thread_t         machine_thread;
    jinue_node_t             thread_list;
    thread_state_t           state;
    process_t               *process;
    struct thread_t         *sender;
    struct thread_t         *awaiter;
    spinlock_t               await_lock;
    addr_t                   local_storage_addr;
    size_t                   local_storage_size;
    size_t                   recv_buffer_size;
    int                      message_errno;
    uintptr_t                message_reply_errcode;
    uintptr_t                message_function;
    uintptr_t                message_cookie;
    size_t                   message_size;
    char                     message_buffer[JINUE_MAX_MESSAGE_SIZE];
};

typedef struct thread_t thread_t;

typedef struct {
    void *entry;
    void *stack_addr;
} thread_params_t;

typedef struct {
    object_header_t header;
    spinlock_t      lock;
    jinue_list_t    send_list;
    jinue_list_t    recv_list;
    int             receivers_count;
} ipc_endpoint_t;

typedef struct {
    void    *start;
    size_t   size;
} exec_file_t;

typedef struct {
    kern_paddr_t    start;
    size_t          size;
} kern_mem_block_t;

typedef struct {
    jinue_node_t loggers;
    void (*log)(int loglevel, const char *message, size_t n);
} logger_t;

typedef enum {
    CONFIG_ON_PANIC_HALT,
    CONFIG_ON_PANIC_REBOOT
} config_on_panic_t;

typedef struct {
    machine_config_t    machine;
    config_on_panic_t   on_panic;
} config_t;

#endif
