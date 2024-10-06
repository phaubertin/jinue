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
#include <kernel/i686/thread.h>
#include <kernel/dup.h>
#include <kernel/object.h>
#include <kernel/process.h>

static int get_process(process_t **process, int process_fd) {
    object_header_t *object;

    int status = process_get_object_header(
            &object,
            NULL,
            process_fd,
            get_current_thread()->process
    );

    if(status < 0) {
        return status;
    }

    if(object->type != OBJECT_TYPE_PROCESS) {
        return -JINUE_EBADF;
    }

    *process = (process_t *)object;
    return 0;
}

int dup(int process_fd, int src, int dest) {
    process_t *process;

    int status = get_process(&process, process_fd);

    if(status < 0) {
        return status;
    }

    object_ref_t *src_ref = process_get_descriptor(get_current_thread()->process, src);

    if(!object_ref_is_in_use(src_ref) || object_ref_is_closed(src_ref)) {
        return -JINUE_EBADF;
    }

    object_ref_t *dest_ref = process_get_descriptor(process, dest);

    if(object_ref_is_in_use(dest_ref)) {
        return -JINUE_EBADF;
    }

    object_addref(src_ref->object);
    dest_ref->object = src_ref->object;
    dest_ref->flags  = src_ref->flags;
    dest_ref->cookie = src_ref->cookie;

    return 0;
}
