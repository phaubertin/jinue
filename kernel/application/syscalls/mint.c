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

static int with_target_process_referenced(
        process_t               *current,
        descriptor_t            *owner_desc,
        descriptor_t            *target_desc,
        const jinue_mint_args_t *args) {
    
    process_t *target = get_process_from_descriptor(target_desc);

    if(target == NULL) {
        return -JINUE_EBADF;
    }

    if(!descriptor_has_permissions(target_desc, JINUE_PERM_OPEN)) {
        return -JINUE_EPERM;
    }

    int status = reserve_unused_descriptor(target, args->fd);

    if(status < 0) {
        return status;
    }

    descriptor_t dest_desc;
    dest_desc.object = owner_desc->object;
    dest_desc.flags  = args->perms;
    dest_desc.cookie = args->cookie;

    open_descriptor(target, args->fd, &dest_desc);

    return 0;
}

static int with_owner_referenced(
        process_t               *current,
        descriptor_t            *owner_desc,
        const jinue_mint_args_t *args) {
    
    int perms       = args->perms;
    int all_perms   = owner_desc->object->type->all_permissions;
    
    if((perms & ~all_perms) != 0) {
        return -JINUE_EINVAL;
    }

    if(perms == 0) {
        return -JINUE_EINVAL;
    }

    if(! descriptor_is_owner(owner_desc)) {
        return -JINUE_EPERM;
    }

    descriptor_t target_desc;
    int status = dereference_object_descriptor(&target_desc, current, args->process);

    if(status < 0) {
        return status;
    }

    status = with_target_process_referenced(current, owner_desc, &target_desc, args);

    unreference_descriptor_object(&target_desc);

    return status;
}

int mint(int owner, const jinue_mint_args_t *args) {
    process_t *current = get_current_process();

    descriptor_t owner_desc;
    int status = dereference_object_descriptor(&owner_desc, current, owner);

    if(status < 0) {
        return status;
    }

    status = with_owner_referenced(current, &owner_desc, args);

    unreference_descriptor_object(&owner_desc);

    return status;
}
