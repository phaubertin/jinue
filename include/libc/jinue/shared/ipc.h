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

#ifndef _JINUE_SHARED_IPC_H_
#define _JINUE_SHARED_IPC_H_

#include <jinue/shared/asm/ipc.h>
#include <jinue/shared/syscall.h>
#include <stddef.h>
#include <stdint.h>

#define JINUE_IPC_NONE      0

#define JINUE_IPC_SYSTEM    (1<<0)

#define JINUE_IPC_PROC      (1<<1)

/* TODO delete this */
typedef int jinue_ipc_descriptor_t;

/* TODO delete this */
static inline uintptr_t jinue_args_pack_buffer_size(size_t buffer_size) {
    return JINUE_ARGS_PACK_BUFFER_SIZE((uintptr_t)buffer_size);
}

/* TODO delete this */
static inline uintptr_t jinue_args_pack_data_size(size_t data_size) {
    return JINUE_ARGS_PACK_DATA_SIZE((uintptr_t)data_size);
}

/* TODO delete this */
static inline uintptr_t jinue_args_pack_n_desc(unsigned int n_desc) {
    return JINUE_ARGS_PACK_N_DESC((uintptr_t)n_desc);
}

/* TODO delete this */
static inline char *jinue_args_get_buffer_ptr(const jinue_syscall_args_t *args) {
    return (char *)(args->arg2);
}

/* TODO delete this */
static inline size_t jinue_args_get_buffer_size(const jinue_syscall_args_t *args) {
    return ((size_t)(args->arg3) >> JINUE_SEND_BUFFER_SIZE_OFFSET) & JINUE_SEND_SIZE_MASK;
}

/* TODO delete this */
static inline size_t jinue_args_get_data_size(const jinue_syscall_args_t *args) {
    return ((size_t)(args->arg3) >> JINUE_SEND_DATA_SIZE_OFFSET) & JINUE_SEND_SIZE_MASK;
}

/* TODO delete this */
static inline unsigned int jinue_args_get_n_desc(const jinue_syscall_args_t *args) {
    return ((unsigned int)(args->arg3) >> JINUE_SEND_N_DESC_OFFSET) & JINUE_SEND_N_DESC_MASK;
}

typedef struct {
    void    *addr;
    size_t   size;
} jinue_buffer_t;

typedef struct {
    const jinue_buffer_t    *send_buffers;
    size_t                   send_buffers_length;
    const jinue_buffer_t    *recv_buffers;
    size_t                   recv_buffers_length;
    uintptr_t                recv_function;
    uintptr_t                recv_cookie;
} jinue_message_t;

#endif
