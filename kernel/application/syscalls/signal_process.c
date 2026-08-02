/*
 * Copyright (C) 2026 Philippe Aubertin.
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
#include <jinue/shared/asm/signal.h>
#include <jinue/shared/asm/permissions.h>
#include <kernel/application/syscalls.h>
#include <kernel/domain/entities/descriptor.h>
#include <kernel/domain/entities/process.h>
#include <kernel/machine/spinlock.h>

int signal_process(int fd, int signo) {
    if(signo < 0 || signo > JINUE_SIGNAL_MAX) {
        return -JINUE_EINVAL;
    }

    descriptor_t desc;
    int status = descriptor_access_object(&desc, get_current_process(), fd);

    if(status < 0) {
        return status == JINUE_EIO ? -JINUE_ESRCH : status;
    }

    process_t *process = descriptor_get_process(&desc);

    if(process == NULL) {
        descriptor_unreference_object(&desc);
        return -JINUE_EBADF;
    }

    if(!descriptor_has_permissions(&desc, JINUE_PERM_SIGNAL)) {
        descriptor_unreference_object(&desc);
        return -JINUE_EPERM;
    }

    if(signo == 0) {
        descriptor_unreference_object(&desc);
        return 0;
    }

    spin_lock(&process->signal_lock);

    sigset_t onemask = 1<<(signo - 1);

    if((process->ignored_signals & onemask) == 0) {
        process->pending_signals |= onemask;
    }

    spin_unlock(&process->signal_lock);
    descriptor_unreference_object(&desc);

    return 0;
}
