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
#include <kernel/application/syscalls.h>
#include <kernel/domain/entities/descriptor.h>
#include <kernel/domain/entities/process.h>
#include <kernel/machine/spinlock.h>
#include <kernel/machine/thread.h>

int get_set_signal_mask(int how, const jinue_sigset_t *set, jinue_sigset_t *oset) {
    thread_t *thread    = get_current_thread();
    process_t *process  = thread->process;

    sigmask_t sigmask;

    if(set == NULL) {
        how = JINUE_SIG_NONE;
    }
    else if (how != JINUE_SIG_NONE) {
        /* Make sure to not dereference set when how is JINUE_SIG_NONE since it
         * is allowed to have any value (e.g. NULL, uninitialized memory) in
         * that case. */
        sigmask = (sigmask_t)set->sa_sigbits[1] << 32 | set->sa_sigbits[0];
    }

    spin_lock(&process->signal_lock);

    sigmask_t original = thread->blocked_signals;

    switch(how) {
        case JINUE_SIG_NONE:
            break;
        case JINUE_SIG_BLOCK:
            thread->blocked_signals |= sigmask;
            break;
        case JINUE_SIG_SETMASK:
            thread->blocked_signals = sigmask;
            break;
        case JINUE_SIG_UNBLOCK:
            thread->blocked_signals &= ~sigmask;
            break;
        default:
            spin_unlock(&process->signal_lock);
            return -JINUE_EINVAL;
    }

    if(oset != NULL) {
        oset->sa_sigbits[0] = original & 0xffffffff;
        oset->sa_sigbits[1] = original >> 32;
        oset->sa_sigbits[2] = 0;
        oset->sa_sigbits[3] = 0;
    }

    spin_unlock(&process->signal_lock);

    return 0;
}
