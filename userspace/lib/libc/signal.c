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
#include <stdlib.h>
#include "pthread/libc.h"
#include "signal.h"

struct sighandler_entry {
    bool is_sigaction;
    union {
        void (*sa_handler)(int);
        void (*sa_sigaction)(int, siginfo_t *, void *);
    } handler;
};

static struct sighandler_entry sighandlers[JINUE_SIGNAL_MAX] = {NULL};

static bool is_reserved_signal(int signo) {
    switch(signo) {
        case SIGCANCEL:
            return true;
        default:
            return false;
    }
}

void __libc_clear_reserved_signals(sigset_t *set) {
    sigdelset(set, SIGCANCEL);
}

static void return_from_signal(void *context) {
    (void)jinue_return_from_signal(context, NULL);

    /* No need to look at the return value of jinue_return_from_signal(), we
     * should not reach this point if it succeeded.
     *
     * TODO we should kill the process instead */
    jinue_exit_thread();
}

static void handle_signal(int signo, jinue_siginfo_t *info, jinue_ucontext_t *context) {
    if(signo < 1 || signo > JINUE_SIGNAL_MAX) {
        return_from_signal(context);
    }

    struct sighandler_entry *entry = &sighandlers[signo - 1];

    if(entry->is_sigaction && entry->handler.sa_sigaction != NULL) {
        entry->handler.sa_sigaction(signo, info, context);
    }
    else if (!entry->is_sigaction && entry->handler.sa_handler != NULL){
        entry->handler.sa_handler(signo);
    }

    return_from_signal(context);
}

static void handle_sigcancel(int signo) {
    /* This solves a dependency issue where the C library needs to set up the
     * SIGCANCEL handler during initialization but the POSIX thread library is
     * a separate static library that isn't guaranteed to have been linked in.
     *  __pthread_handle_sigcancel is a function pointer in libc that is called
     * by this signal handler stub and it gets assigned by the POSIX thread
     * ibrary (more specifically ptthread_cancel()) on first use. */
    if(__pthread_handle_sigcancel != NULL) {
        __pthread_handle_sigcancel(signo);
    }
}
int raise(int sig) {
    return jinue_signal_thread(-1, sig, &errno);
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

static int do_sigaction(int sig, const struct sigaction *restrict act, struct sigaction *restrict oact) {
    /* The implementation of signals in the kernel is currently incomplete.
     * This function is similarly incomplete and only allows setting a signal
     * handler, which is the only action the kernel supports. */
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

int sigaction(int sig, const struct sigaction *restrict act, struct sigaction *restrict oact) {
    if(is_reserved_signal(sig)) {
        act = NULL;
    }

    return do_sigaction(sig, act, oact);
}

int sigaddset(sigset_t *set, int signo) {
    return jinue_sigaddset(set, signo, &errno);
}

int sigdelset(sigset_t *set, int signo) {
    return jinue_sigdelset(set, signo, &errno);
}

int sigemptyset(sigset_t *set) {
    jinue_sigemptyset(set);
    return 0;
}

int sigfillset(sigset_t *set) {
    jinue_sigfillset(set);
    return 0;
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

    sigset_t local_set;
    const sigset_t *iset;

    if(set == NULL || how == SIG_UNBLOCK) {
        iset = set;
    }
    else {
        iset = &local_set;
        jinue_sigcopyset(&local_set, set);
        __libc_clear_reserved_signals(&local_set);
    }

    return jinue_get_set_signal_mask(how, iset, oset, &errno);
}

int __libc_signal_init(void) {
    int status = jinue_set_signal_handler(handle_signal, NULL);

    if(status < 0) {
        return EXIT_FAILURE;
    }

    struct sigaction act;
    act.sa_flags    = 0;
    act.sa_handler  = handle_sigcancel;
    
    status = sigemptyset(&act.sa_mask);

    if(status != 0) {
        return EXIT_FAILURE;
    }

    status = do_sigaction(SIGCANCEL, &act, NULL);

    if(status != 0) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
