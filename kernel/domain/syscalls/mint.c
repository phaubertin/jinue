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
#include <kernel/descriptor.h>
#include <kernel/object.h>
#include <kernel/process.h>

static int check_mint_permissions(const object_header_t *object, int perms) {
    if((perms & ~object->type->all_permissions) != 0) {
        return -JINUE_EINVAL;
    }

    /* TODO remove this once permissions are defined for process objects */
    if(object->type == object_type_process) {
        return 0;
    }

    if(perms == 0) {
        return -JINUE_EINVAL;
    }

    return 0;
}

int mint(int owner, const jinue_mint_args_t *mint_args) {
    object_header_t *object;
    object_ref_t    *src_ref;
    
    int status = dereference_object_descriptor(&object, &src_ref, get_current_process(), owner);

    if(status < 0) {
        return status;
    }

    status = check_mint_permissions(object, mint_args->perms);

    if(status < 0) {
        return status;
    }

    if(!object_ref_is_owner(src_ref)) {
        return -JINUE_EPERM;
    }

    process_t *process;

    status = get_process(&process, mint_args->process);

    if(status < 0) {
        return status;
    }

    object_ref_t *dest_ref;
    
    status = dereference_unused_descriptor(&dest_ref, process, mint_args->fd);

    if(status < 0) {
        return status;
    }

    dest_ref->object = src_ref->object;
    dest_ref->flags  =
          mint_args->perms
        | (src_ref->flags & OBJECT_FLAG_DESTROYED)
        | OBJECT_REF_FLAG_IN_USE;
    dest_ref->cookie = mint_args->cookie;

    object_open(object, dest_ref);

    return 0;
}
