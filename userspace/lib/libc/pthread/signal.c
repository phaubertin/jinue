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

#include <jinue/jinue.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include "../signal.h"
#include "thread.h"

int pthread_kill(pthread_t thread, int sig) {
    int errno_retval;
    
    int status = jinue_signal_thread(thread->fd, sig, &errno_retval);

    if(status < 0) {
        return errno_retval;
    }

    return 0;
}

int pthread_sigmask(int how, const sigset_t *restrict set, sigset_t *restrict oset) {
    /* In addition to the valid POSIX SIG_... values, the system call accepts a
     * JINUE_SIG_NONE value that allows the caller to be more explicit about
     * not wanting to make changes. We must reject this value since POSIX has
     * no equivalent. */
    if(how == JINUE_SIG_NONE) {
        return EINVAL;
    }

    sigset_t local_set;
    const sigset_t *iset;

    if(set == NULL || how == SIG_UNBLOCK) {
        iset = set;
    }
    else {
        iset = &local_set;
        local_set = *set;
        __libc_clear_reserved_signals(&local_set);
    }

    int errno_retval;

    int status = jinue_get_set_signal_mask(how, iset, oset, &errno_retval);

    if(status < 0) {
        return errno_retval;
    }

    return 0;
}
