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
#include <signal.h>
#include <stdbool.h>

struct sighandler_entry {
    bool is_sigaction;
    union {
        void (*sa_handler)(int);
        void (*sa_sigaction)(int, siginfo_t *, void *);
    } handler;
};

static struct sighandler_entry sighandlers[JINUE_SIGNAL_MAX] = {NULL};

int raise(int sig) {
    /* The JINUE_SIG_FLAG_SYNC flag is needed in order to meet this POSIX
     * requirement:
     *
     *  " If a signal handler is called, the raise() function shall not return
     *    until after the signal handler does."
     */
    return jinue_signal_thread(-1, sig, JINUE_SIG_FLAG_SYNC, &errno);
}

static int update_sighandler_entry(struct sighandler_entry *entry, const struct sigaction *act) {
    if((act->sa_flags & ~SA_SIGINFO) != 0) {
        errno = ENOTSUP;
        return -1;
    }

    if(act->sa_flags & SA_SIGINFO) {
        entry->is_sigaction = true;
        entry->handler.sa_sigaction = act->sa_sigaction;
        return 0;
    }
    
    if(act->sa_handler == SIG_DFL || act->sa_handler == SIG_ERR || act->sa_handler == SIG_IGN) {
        errno = ENOTSUP;
        return -1;
    }

    entry->is_sigaction = false;
    entry->handler.sa_handler = act->sa_handler;

    return 0;
}

int sigaction(int sig, const struct sigaction *restrict act, struct sigaction *restrict oact) {
    /* The implementation of signals in the kernel is currently incomplete.
     * This function is similarly incomplete and only allows setting a signal
     * handler which is the only action the kernel supports. */

    if(sig < 1 || sig > JINUE_SIGNAL_MAX) {
        errno = EINVAL;
        return -1;
    }
    
    struct sighandler_entry *entry = &sighandlers[sig - 1];
    struct sighandler_entry original = *entry;

    if(act != NULL) {
        int status = update_sighandler_entry(entry, act);

        if(status != 0) {
            return status;
        }
    }

    if(oact != NULL) {
        sigemptyset(&oact->sa_mask);

        if(original.is_sigaction) {
            oact->sa_flags = SA_SIGINFO;
            oact->sa_handler = NULL;
            oact->sa_sigaction = original.handler.sa_sigaction;
        }
        else {
            oact->sa_flags = 0;
            oact->sa_handler = original.handler.sa_handler;
            oact->sa_sigaction = NULL;
        }
    }

    return 0;
}

int sigaddset(sigset_t *set, int signo) {
    return jinue_sigaddset(set, signo, &errno);
}

int sigdelset(sigset_t *set, int signo) {
    return jinue_sigdelset(set, signo, &errno);
}

int sigemptyset(sigset_t *set) {
    return jinue_sigemptyset(set);
}

int sigfillset(sigset_t *set) {
    return jinue_sigfillset(set);
}

int sigismember(const sigset_t *set, int signo) {
    return jinue_sigismember(set, signo, &errno);
}

int sigprocmask(int how, const sigset_t *restrict set, sigset_t *restrict oset) {
    /* In addition to the valid POSIX SIG_... values, the system call accepts a
     * JINUE_SIG_NONE value that allows the caller to be more explicit about
     * not wanting to make changes. We must reject this value since POSIX has
     * no equivalent. */
    if(how == JINUE_SIG_NONE) {
        return EINVAL;
    }

    return jinue_get_set_signal_mask(how, set, oset, &errno);
}
