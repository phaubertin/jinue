/*
 * Copyright (C) 2017 Philippe Aubertin.
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

#ifndef _JINUE_IPC_H_
#define _JINUE_IPC_H_

#include <jinue-common/ipc.h>
#include <stddef.h>

typedef struct {
    uintptr_t            function;
    uintptr_t            cookie;
    size_t               buffer_size;
    size_t               data_size;
    size_t               desc_n;
} jinue_message_t;

typedef struct {
    size_t               data_size;
    size_t               desc_n;
} jinue_reply_t;

int jinue_send(
        int              function,
        int              fd,
        char            *buffer,
        size_t           buffer_size,
        size_t           data_size,
        unsigned int     n_desc,
        int             *perrno);

int jinue_receive(
        int              fd,
        char            *buffer,
        size_t           buffer_size,
        jinue_message_t *message,
        int             *perrno);

int jinue_reply(
        char            *buffer,
        size_t           buffer_size,
        size_t           data_size,
        unsigned int     n_desc,
        int             *perrno);

int jinue_create_ipc(int flags, int *perrno);

#endif
