/*
 * Copyright (C) 2024-2026 Philippe Aubertin.
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

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include "../signal.h"
#include "./libc.h"
#include "./thread.h"

static void do_testcancel(bool async_only) {
    pthread_t thread = pthread_self();

    if(!thread->is_cancel_requested) {
        return;
    }

    if(thread->cancel_state == PTHREAD_CANCEL_DISABLE) {
        return;
    }

    if(async_only && thread->cancel_type == PTHREAD_CANCEL_DEFERRED) {
        return;
    }

    pthread_exit(PTHREAD_CANCELED);
}

static void handle_sigcancel(void) {
    pthread_t thread = pthread_self();
    
    thread->is_cancel_requested = true;
    
    do_testcancel(true);
}

int pthread_cancel(pthread_t thread) {
    /* This solves a dependency issue where the C library needs to set up the
     * SIGCANCEL handler during initialization but the POSIX thread library is
     * a separate static library that isn't guaranteed to have been linked in.
     *  __pthread_handle_sigcancel is a function pointer in libc that is called
     * by a signal handler stub set up during initialization and we assign it
     * here on first use. */
    __pthread_handle_sigcancel = handle_sigcancel;

    /* TODO do we need as barrier here? */
    
    return pthread_kill(thread, SIGCANCEL);
}

void pthread_testcancel(void) {
    do_testcancel(false);
}

int pthread_setcancelstate(int state, int *oldstate) {
    pthread_t thread = pthread_self();

    switch(state) {
        case PTHREAD_CANCEL_DISABLE:
        case PTHREAD_CANCEL_ENABLE:
            break;
        default:
            thread->local_errno = EINVAL;
            return -1;
    }

    if(oldstate) {
        *oldstate = thread->cancel_state;
    }

    thread->cancel_state = state;

    do_testcancel(true);

    return 0;
}

int pthread_setcanceltype(int type, int *oldtype) {
    pthread_t thread = pthread_self();

    switch(type) {
        case PTHREAD_CANCEL_DEFERRED:
        case PTHREAD_CANCEL_ASYNCHRONOUS:
            break;
        default:
            thread->local_errno = EINVAL;
            return -1;
    }

    if(oldtype) {
        *oldtype = thread->cancel_type;
    }

    thread->cancel_type = type;

    do_testcancel(true);

    return 0;
}
