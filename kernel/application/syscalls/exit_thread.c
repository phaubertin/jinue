/*
 * Copyright (C) 2019-2024 Philippe Aubertin.
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
#include <kernel/domain/entities/thread.h>
#include <kernel/domain/services/ipc.h>
#include <kernel/machine/thread.h>

void exit_thread(void *exit_value) {
    thread_t *thread = get_current_thread();

    if(thread->sender != NULL) {
        abort_message(thread->sender);
        thread->sender = NULL;
    }

    thread->exit_value = exit_value;

    /* When we started the thread in start_thread(), we incremented the
     * reference count on it to ensure it continues running even if all
     * descriptors that reference it are closed. This call here will safely
     * decrement the reference count after it has switched to another thread.
     * We cannot just decrement the count here because that will possibly free
     * the current thread, which we don't want to do while it is still running.
     * 
     * This call must be the last thing happening in this function. */
    switch_from_exiting_thread(thread);
}
