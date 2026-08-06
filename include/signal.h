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

#ifndef _JINUE_LIBC_SIGNAL_H
#define _JINUE_LIBC_SIGNAL_H

#include <jinue/shared/types.h>
#include <pthread.h>
#include <stdint.h>

#define SIG_BLOCK   JINUE_SIG_BLOCK

#define SIG_SETMASK JINUE_SIG_SETMASK

#define SIG_UNBLOCK JINUE_SIG_UNBLOCK


#define SA_NOCLDSTOP    (1<<0)

#define SA_ONSTACK      (1<<1)

#define SA_RESETHAND    (1<<2)

#define SA_RESTART      (1<<3)

#define SA_SIGINFO      (1<<4)

#define SA_NOCLDWAIT    (1<<5)

#define SA_NODEFER      (1<<6)

#define SIG_DFL         (void (*)(int))-1

#define SIG_ERR         (void (*)(int))-2

#define SIG_IGN         (void (*)(int))-3


typedef jinue_sigset_t sigset_t;

typedef jinue_siginfo_t siginfo_t;

struct sigaction {
    void   (*sa_handler)(int);
    sigset_t sa_mask;
    int      sa_flags;
    void   (*sa_sigaction)(int, siginfo_t *, void *);
};


int pthread_kill(pthread_t thread, int sig);

int pthread_sigmask(int how, const sigset_t *restrict set, sigset_t *restrict oset);

/* TODO raise() */

int sigaction(int sig, const struct sigaction *restrict act, struct sigaction *restrict oact);

int sigaddset(sigset_t *set, int signo);

int sigdelset(sigset_t *set, int signo);

int sigemptyset(sigset_t *set);

int sigfillset(sigset_t *set);

int sigismember(const sigset_t *set, int signo);

int sigprocmask(int how, const sigset_t *restrict set, sigset_t *restrict oset);

#endif
