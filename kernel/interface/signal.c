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

#include <kernel/interface/machine/signal.h>
#include <kernel/interface/machine/trap.h>
#include <kernel/interface/signal.h>
#include <kernel/machine/spinlock.h>
#include <kernel/machine/thread.h>
#include <kernel/types.h>

/**
 * Check for pending signals the current thread should handle
 * 
 * This is called just before returning to user space from a system call or
 * interrupt. If there is an unblocked pending on the thread or process, this
 * function updates the set of pending signals and the signal mask, and then
 * calls the architecture-specific deliver_signal() function to actually
 * deliver the signal.
 * 
 * @param trapframe trap frame
 */
void check_for_signal(trapframe_t *trapframe) {
    thread_t *thread = get_current_thread();
    process_t *process = thread->process;

    spin_lock(&process->signal_lock);

    sigmask_t sigmask = thread->blocked_signals;
    sigmask_t pending = process->pending_signals | thread->pending_signals;
    sigmask_t signals = pending & ~sigmask;

    if(signals == 0 && thread->sync_signo == 0) {
        spin_unlock(&process->signal_lock);
        return;
    }

    /* We cannot deliver a signal if a signal handler has not been set. */
    if(process->signal_handler == NULL) {
        spin_unlock(&process->signal_lock);
        return;
    }

    int signo;
    sigmask_t onemask;

    if(thread->sync_signo != 0) {
        /* If a signal is generated synchronously, i.e. by a CPU exception,
         * this signal bypasses all others and is delivered immediately. */
        signo = thread->sync_signo;        
        onemask = (sigmask_t)1<<(signo - 1);

        thread->sync_signo = 0;
    }
    else {
        signo = 1;
        onemask = 1;

        /* This loop condition is potentially dangerous but we checked signals is
         * not zero above, so the loop is guaranteed to terminate. */
        while((signals & onemask) == 0) {
            signo += 1;
            onemask <<= 1;
        }

        if(thread->pending_signals & onemask) {
            thread->pending_signals &= ~onemask;
        }
        else {
            process->pending_signals &= ~onemask;
        }
    }

    thread->blocked_signals |= onemask;

    spin_unlock(&process->signal_lock);

    deliver_signal(trapframe, signo, sigmask);
}
