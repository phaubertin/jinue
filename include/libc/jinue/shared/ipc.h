/*
 * Copyright (C) 2019-2022 Philippe Aubertin.
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

typedef struct {
    void        *addr;
    size_t       size;
} jinue_buffer_t;

typedef struct {
    const void  *addr;
    size_t       size;
} jinue_const_buffer_t;

typedef struct {
    const jinue_const_buffer_t  *send_buffers;
    size_t                       send_buffers_length;
    const jinue_buffer_t        *recv_buffers;
    size_t                       recv_buffers_length;
    uintptr_t                    recv_function;
    uintptr_t                    recv_cookie;
    uintptr_t                    reply_max_size;
} jinue_message_t;

#endif
