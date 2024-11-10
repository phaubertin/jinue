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

#ifndef _JINUE_LIBC_PTHREAD_H
#define _JINUE_LIBC_PTHREAD_H

#include <stddef.h>

#define PTHREAD_STACK_MIN 16384

typedef struct __pthread *pthread_t;

typedef struct {
    int      flags;
    int      detachstate;
    size_t   stacksize;
    void    *stackaddr;
} pthread_attr_t;

/* threads */

pthread_t pthread_self(void);

int pthread_create(
        pthread_t               *restrict thread,
        const pthread_attr_t    *restrict attr,
        void                    *(*start_routine)(void*),
        void                    *restrict arg);

int pthread_join(pthread_t thread, void **exit_status);

void pthread_exit(void *exit_status);

/* thread attributes */

int pthread_attr_destroy(pthread_attr_t *attr);

int pthread_attr_init(pthread_attr_t *attr);

#define PTHREAD_CREATE_JOINABLE 0

#define PTHREAD_CREATE_DETACHED 1

int pthread_attr_getdetachstate(const pthread_attr_t *attr, int *detachstate);

int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate);

int pthread_attr_getstacksize(const pthread_attr_t *restrict attr, size_t *restrict stacksize);

int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize);

int pthread_attr_getstack(
        const pthread_attr_t     *restrict attr,
        void                    **restrict stackaddr,
        size_t                   *restrict stacksize);

int pthread_attr_setstack(pthread_attr_t *attr, void *stackaddr, size_t stacksize);

#endif
