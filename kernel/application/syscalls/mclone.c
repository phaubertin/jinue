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
#include <kernel/domain/entities/process.h>
#include <kernel/domain/services/logging.h>
#include <kernel/machine/pmap.h>

/**
 * Implementation for the MCLONE system call
 *
 * Clone memory mappings from one process to another.
 *
 * @param src source process descriptor number
 * @param dest destination process descriptor number
 * @param args MCLONE system call arguments structure
 * @return zero on success, negated error code on failure
 *
 */
int mclone(int src, int dest, const jinue_mclone_args_t *args) {
    process_t *current_process = get_current_process();

    descriptor_t *src_desc;
    int status = dereference_object_descriptor(&src_desc, current_process, src);
    
    if(status < 0) {
        return status;
    }
    
    process_t *src_process = get_process_from_descriptor(src_desc);

    if(src_process == NULL) {
        return -JINUE_EBADF;
    }

    /* TODO what permissions do we need on the source for this? Should the
     * source just implicitly be the current process? */
    
    descriptor_t *dest_desc;
    status = dereference_object_descriptor(&dest_desc, current_process, dest);
    
    if(status < 0) {
        return status;
    }
    
    process_t *dest_process = get_process_from_descriptor(dest_desc);

    if(dest_process == NULL) {
        return -JINUE_EBADF;
    }

    if(!descriptor_has_permissions(dest_desc, JINUE_PERM_MAP)) {
        return -JINUE_EPERM;
    }

    bool success = machine_clone_userspace_mapping(
        dest_process,
        args->dest_addr,
        src_process,
        args->src_addr,
        args->length,
        args->prot
    );

    if(!success) {
        return -JINUE_ENOMEM;
    }

    return 0;
}
