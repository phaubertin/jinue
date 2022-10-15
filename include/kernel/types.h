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
#include <jinue/shared/list.h>
#include <jinue/shared/syscall.h>
#include <jinue/shared/types.h>
#include <hal/types.h>
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

typedef struct {
    int type;
    int ref_count;
    int flags;
} object_header_t;

typedef struct {
    object_header_t *object;
    uintptr_t        flags;
    uintptr_t        cookie;
} object_ref_t;

#define PROCESS_MAX_DESCRIPTORS     12

typedef struct {
    object_header_t header;
    addr_space_t    addr_space;
    object_ref_t    descriptors[PROCESS_MAX_DESCRIPTORS];
} process_t;

/* TODO delete this */
typedef struct {
    void    *user_ptr;
    size_t   buffer_size;
    size_t   data_size;
    size_t   desc_n;
    size_t   total_size;
} syscall_input_buffer_t;

struct thread_t {
    object_header_t          header;
    thread_context_t         thread_ctx;
    jinue_node_t             thread_list;
    process_t               *process;
    addr_t                   local_storage_addr;
    size_t                   local_storage_size;
    struct thread_t         *sender;
    /* TODO get rid of this member*/
    jinue_syscall_args_t    *message_args;
    size_t                   recv_buffer_size;
    size_t                   message_size;
    size_t                   reply_size;
    int                      reply_errno;
    char                     message_buffer[JINUE_SEND_MAX_SIZE];
};

typedef struct thread_t thread_t;

typedef struct {
    object_header_t header;
    jinue_list_t    send_list;
    jinue_list_t    recv_list;
} ipc_t;

#endif
