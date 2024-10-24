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
#include <kernel/application/syscalls.h>
#include <kernel/domain/entities/descriptor.h>
#include <kernel/domain/entities/object.h>
#include <kernel/domain/entities/process.h>
#include <kernel/domain/entities/thread.h>
#include <kernel/machine/thread.h>

int join_thread(int fd, void **exit_value) {
    descriptor_t *desc;
    int status = dereference_object_descriptor(&desc, get_current_process(), fd);

    if(status < 0) {
        return -JINUE_EBADF;
    }

    thread_t *thread = get_thread_from_descriptor(desc);

    if(thread == NULL) {
        return -JINUE_EBADF;
    }

    if(!descriptor_has_permissions(desc, JINUE_PERM_JOIN)) {
        return -JINUE_EPERM;
    }

    thread_t *current = get_current_thread();

    if(thread == current) {
        return -JINUE_EDEADLK;
    }

    if(thread->joined != NULL) {
        return -JINUE_ESRCH;
    }

    thread->joined = current;

    /* Keep the thread around until we actually read the exit value. */
    add_ref_to_object(&thread->header);

    if(thread->state != THREAD_STATE_ZOMBIE) {
        block_current_thread();
    }

    *exit_value = thread->exit_value;

    sub_ref_to_object(&thread->header);

    return 0;
}