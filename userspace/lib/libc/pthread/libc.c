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

#include <jinue/jinue.h>
#include <pthread.h>
#include <stdlib.h>
#include "libc.h"
#include "thread.h"

/* This file contains the subset of the POSIX threads implementation that is in
 * libc instead of libpthread because of dependencies in the libc initilization
 * code.*/

static struct __pthread main_thread = {
    .self               = &main_thread,
    .next               = NULL,
    .fd                 = JINUE_DESC_MAIN_THREAD,
    .flags              = THREAD_FLAG_RUNNING,
    .stackaddr          = (void *)JINUE_STACK_START,
    .stacksize          = JINUE_STACK_SIZE,
    .alloc_stackaddr    = (void *)JINUE_STACK_START,
    .alloc_stacksize    = JINUE_STACK_SIZE,
    .exit_status        = NULL
};

pthread_t __pthread_main_thread = &main_thread;

void __pthread_set_current(pthread_t thread) {
    jinue_set_thread_local(thread, sizeof(struct __pthread));
}

pthread_t pthread_self(void) {
    return jinue_get_thread_local();
}
