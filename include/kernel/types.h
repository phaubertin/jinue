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

#include <jinue/shared/ipc.h>
#include <jinue/shared/syscall.h>
#include <jinue/shared/types.h>
#include <kernel/machine/types.h>
#include <kernel/list.h>
#include <sys/elf.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

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

typedef struct object_ref_t object_ref_t;

typedef struct object_header_t object_header_t;

typedef void (*object_ref_func_t)(object_header_t *, const object_ref_t *);

typedef struct {
    int                  all_permissions;
    char                *name;
    size_t               size;
    object_ref_func_t    open;
    object_ref_func_t    close;
    slab_ctor_t          cache_ctor;
    slab_ctor_t          cache_dtor;
} object_type_t;

struct object_header_t {
    const object_type_t *type;
    int                  ref_count;
    int                  flags;
};

struct object_ref_t {
    object_header_t *object;
    uintptr_t        flags;
    uintptr_t        cookie;
};

#define PROCESS_MAX_DESCRIPTORS     12

typedef struct {
    object_header_t header;
    addr_space_t    addr_space;
    object_ref_t    descriptors[PROCESS_MAX_DESCRIPTORS];
} process_t;

struct thread_t {
    object_header_t          header;
    machine_thread_t         thread_ctx;
    jinue_node_t             thread_list;
    process_t               *process;
    addr_t                   local_storage_addr;
    size_t                   local_storage_size;
    struct thread_t         *sender;
    size_t                   recv_buffer_size;
    int                      reply_errno;
    uintptr_t                message_function;
    uintptr_t                message_cookie;
    size_t                   message_size;
    char                     message_buffer[JINUE_MAX_MESSAGE_SIZE];
};

typedef struct thread_t thread_t;

typedef struct {
    object_header_t header;
    jinue_list_t    send_list;
    jinue_list_t    recv_list;
    int             receivers_count;
} ipc_endpoint_t;

typedef struct {
    Elf32_Ehdr  *ehdr;
    size_t       size;
} elf_file_t;

typedef struct {
    kern_paddr_t    start;
    size_t          size;
} kern_mem_block_t;

typedef struct {
    jinue_node_t loggers;
    void (*log)(int loglevel, const char *message, size_t n);
} logger_t;

#endif
