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


static int with_source_referenced(
        process_t       *current,
        process_t       *target,
        descriptor_t    *src_desc,
        int              dest) {

    if(descriptor_is_owner(src_desc)) {
        return -JINUE_EBADF;
    }

    int status = reserve_free_descriptor(target, dest);

    if(status < 0) {
        return status;
    }

    open_descriptor(target, dest, src_desc);

    return 0;
}

static int with_target_process_referenced(
        process_t       *current,
        descriptor_t    *target_desc,
        int              src,
        int              dest) {
    
    process_t *target = get_process_from_descriptor(target_desc);

    if(target == NULL) {
        return -JINUE_EBADF;
    }

    if(!descriptor_has_permissions(target_desc, JINUE_PERM_OPEN)) {
        return -JINUE_EPERM;
    }

    descriptor_t src_desc;
    int status = dereference_object_descriptor(&src_desc, current, src);

    if(status < 0) {
        return status;
    }

    status = with_source_referenced(current, target, &src_desc, dest);

    unreference_descriptor_object(&src_desc);

    return status;
}

int dup(int process_fd, int src, int dest) {
    process_t *current = get_current_process();

    descriptor_t target_desc;
    int status = dereference_object_descriptor(&target_desc, current, process_fd);
    
    if(status < 0) {
        return status;
    }

    status = with_target_process_referenced(current, &target_desc, src, dest);

    unreference_descriptor_object(&target_desc);

    return status;
}
