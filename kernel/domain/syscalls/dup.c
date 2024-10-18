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

#include <jinue/shared/asm/errno.h>
#include <kernel/domain/entities/descriptor.h>
#include <kernel/domain/entities/object.h>
#include <kernel/domain/entities/process.h>
#include <kernel/domain/syscalls.h>

int dup(int process_fd, int src, int dest) {
    process_t *current_process = get_current_process();

    descriptor_t *process_desc;
    int status = dereference_object_descriptor(&process_desc, current_process, process_fd);
    
    if(status < 0) {
        return status;
    }
    
    process_t *process = get_process_from_descriptor(process_desc);

    if(process == NULL) {
        return -JINUE_EBADF;
    }

    descriptor_t *src_desc;
    status = dereference_object_descriptor(&src_desc, current_process, src);

    if(status < 0) {
        return status;
    }

    object_header_t *object = src_desc->object;

    if(descriptor_is_owner(src_desc)) {
        return -JINUE_EBADF;
    }

    descriptor_t *dest_desc;
    status = dereference_unused_descriptor(&dest_desc, process, dest);

    if(status < 0) {
        return status;
    }

    dest_desc->object = src_desc->object;
    dest_desc->flags  = src_desc->flags;
    dest_desc->cookie = src_desc->cookie;

    object_open(object, dest_desc);

    return 0;
}
