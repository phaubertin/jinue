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

#include <jinue/memory.h>
#include <jinue/ipc.h>
#include <jinue/syscall.h>

const char *jinue_phys_mem_type_description(uint32_t type) {
    switch(type) {

    case E820_RAM:
        return "Available";

    case E820_RESERVED:
        return "Unavailable/Reserved";

    case E820_ACPI:
        return "Unavailable/ACPI";

    default:
        return "Unavailable/Other";
    }
}

int jinue_get_user_memory(jinue_mem_map_t *buffer, size_t buffer_size, int *perrno) {
    jinue_syscall_args_t args;

    /* Silently crop the buffer size if it is greater than the maximum allowed. */
    if(buffer_size > JINUE_SEND_MAX_SIZE) {
        buffer_size = JINUE_SEND_MAX_SIZE;
    }

    args.arg0 = SYSCALL_FUNC_GET_USER_MEMORY;
    args.arg1 = 0;
    args.arg2 = (uintptr_t)buffer;
    args.arg3 = jinue_args_pack_buffer_size(buffer_size);

    return jinue_syscall_with_usual_convention(&args, perrno);
}
