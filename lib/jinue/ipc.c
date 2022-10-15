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

#include <jinue/errno.h>
#include <jinue/ipc.h>
#include <jinue/syscall.h>

intptr_t jinue_send(
        int              function,
        int              fd,
        char            *buffer,
        size_t           buffer_size,
        size_t           data_size,
        unsigned int     n_desc,
        int             *perrno) {

    jinue_syscall_args_t args;

    /* The library has to perform this check and set the appropriate error
     * because the kernel cannot check this once the values have been packed. */
    if(data_size > JINUE_SEND_MAX_SIZE || n_desc > JINUE_SEND_MAX_N_DESC) {
        jinue_set_errno(perrno, JINUE_EINVAL);
        return -1;
    }

    /* Silently crop the buffer size if it is greater than the maximum allowed. */
    if(buffer_size > JINUE_SEND_MAX_SIZE) {
        buffer_size = JINUE_SEND_MAX_SIZE;
    }

    /** TODO how do we deal with negative function numbers? Is int the right type?*/
    args.arg0 = (uintptr_t)function;
    args.arg1 = (uintptr_t)fd;
    args.arg2 = (uintptr_t)buffer;
    args.arg3 =     jinue_args_pack_buffer_size(buffer_size)
                  | jinue_args_pack_data_size(data_size)
                  | jinue_args_pack_n_desc(n_desc);

    return jinue_syscall_with_usual_convention(&args, perrno);

    /** TODO handle reply data size/n_desc */
}

intptr_t jinue_receive(
        int              fd,
        char            *buffer,
        size_t           buffer_size,
        jinue_message_t *message,
        int             *perrno) {

    jinue_syscall_args_t args;

    /* Silently crop the buffer size if it is greater than the maximum allowed. */
    if(buffer_size > JINUE_SEND_MAX_SIZE) {
        buffer_size = JINUE_SEND_MAX_SIZE;
    }

    args.arg0 = SYSCALL_FUNC_RECEIVE;
    args.arg1 = (uintptr_t)fd;
    args.arg2 = (uintptr_t)buffer;
    args.arg3 = jinue_args_pack_buffer_size(buffer_size);

    intptr_t retval = jinue_syscall_with_usual_convention(&args, perrno);

    if(retval < 0) {
        return retval;
    }

    message->function       = args.arg0;
    message->cookie         = args.arg1;
    message->buffer_size    = jinue_args_get_buffer_size(&args);
    message->data_size      = jinue_args_get_data_size(&args);
    message->desc_n         = jinue_args_get_n_desc(&args);

    return 0;
}

intptr_t jinue_reply(
        char            *buffer,
        size_t           buffer_size,
        size_t           data_size,
        unsigned int     n_desc,
        int             *perrno) {

    jinue_syscall_args_t args;

    /* The library has to perform this check and set the appropriate error
     * because the kernel cannot check this once the values have been packed. */
    if(data_size > JINUE_SEND_MAX_SIZE || n_desc > JINUE_SEND_MAX_N_DESC) {
        jinue_set_errno(perrno, JINUE_EINVAL);
        return -1;
    }

    /* Silently crop the buffer size if it is greater than the maximum allowed. */
    if(buffer_size > JINUE_SEND_MAX_SIZE) {
        buffer_size = JINUE_SEND_MAX_SIZE;
    }

    args.arg0 = SYSCALL_FUNC_REPLY;
    args.arg1 = 0;
    args.arg2 = (uintptr_t)buffer;
    args.arg3 =     jinue_args_pack_buffer_size(buffer_size)
                  | jinue_args_pack_data_size(data_size)
                  | jinue_args_pack_n_desc(n_desc);

    return jinue_syscall_with_usual_convention(&args, perrno);
}

int jinue_create_ipc(int flags, int *perrno) {
    jinue_syscall_args_t args;

    args.arg0 = SYSCALL_FUNC_CREATE_IPC;
    args.arg1 = (uintptr_t)flags;
    args.arg2 = 0;
    args.arg3 = 0;

    return jinue_syscall_with_usual_convention(&args, perrno);
}
