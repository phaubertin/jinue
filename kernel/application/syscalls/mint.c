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

static int check_mint_permissions(const object_header_t *object, int perms) {
    if((perms & ~object->type->all_permissions) != 0) {
        return -JINUE_EINVAL;
    }

    if(perms == 0) {
        return -JINUE_EINVAL;
    }

    return 0;
}

int mint(int owner, const jinue_mint_args_t *mint_args) {
    process_t *current_process = get_current_process();

    descriptor_t *src_desc;
    int status = dereference_object_descriptor(&src_desc, current_process, owner);

    if(status < 0) {
        return status;
    }

    object_header_t *object = src_desc->object;

    status = check_mint_permissions(object, mint_args->perms);

    if(status < 0) {
        return status;
    }

    if(!descriptor_is_owner(src_desc)) {
        return -JINUE_EPERM;
    }

    descriptor_t *process_desc;
    status = dereference_object_descriptor(&process_desc, current_process, mint_args->process);

    process_t *process = get_process_from_descriptor(process_desc);

    if(process == NULL) {
        return -JINUE_EBADF;
    }

    if(!descriptor_has_permissions(process_desc, JINUE_PERM_OPEN)) {
        return -JINUE_EPERM;
    }

    descriptor_t *dest_desc;
    status = dereference_unused_descriptor(&dest_desc, process, mint_args->fd);

    if(status < 0) {
        return status;
    }

    dest_desc->object = src_desc->object;
    dest_desc->flags  =
          mint_args->perms
        | (src_desc->flags & OBJECT_FLAG_DESTROYED)
        | DESCRIPTOR_FLAG_IN_USE;
    dest_desc->cookie = mint_args->cookie;

    open_object(object, dest_desc);

    return 0;
}
