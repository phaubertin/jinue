/*
 * Copyright (C) 2019 Philippe Aubertin.
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

#include <jinue/errno.h>
#include <jinue/ipc.h>
#include <jinue/syscall.h>
#include <stdbool.h>
#include "stubs.h"

static jinue_syscall_stub_t syscall_stubs[] = {
        [JINUE_SYSCALL_IMPL_INTERRUPT]         = jinue_syscall_intr,
        [JINUE_SYSCALL_IMPL_FAST_AMD]     = jinue_syscall_fast_amd,
        [JINUE_SYSCALL_IMPL_FAST_INTEL]   = jinue_syscall_fast_intel
};

static int syscall_stub_index = JINUE_SYSCALL_IMPL_INTERRUPT;

uintptr_t jinue_syscall(jinue_syscall_args_t *args) {
    return syscall_stubs[syscall_stub_index](args);
}

intptr_t jinue_syscall_with_usual_convention(jinue_syscall_args_t *args, int *perrno) {
    const intptr_t retval = (intptr_t)jinue_syscall(args);

    if(retval < 0) {
        jinue_set_errno(perrno, args->arg1);
    }

    return retval;
}

int jinue_set_syscall_implementation(int implementation, int *perrno) {
    if(implementation < 0 || implementation > JINUE_SYSCALL_IMPL_LAST) {
        *perrno = JINUE_EINVAL;
        return -1;
    }

    syscall_stub_index = implementation;
    return 0;
}

void jinue_set_thread_local(void *addr, size_t size) {
    jinue_syscall_args_t args;

    args.arg0 = JINUE_SYS_SET_THREAD_LOCAL;
    args.arg1 = (uintptr_t)addr;
    args.arg2 = (uintptr_t)size;
    args.arg3 = 0;

    jinue_syscall(&args);
}

void *jinue_get_thread_local(void) {
    jinue_syscall_args_t args;

    args.arg0 = JINUE_SYS_GET_THREAD_LOCAL;
    args.arg1 = 0;
    args.arg2 = 0;
    args.arg3 = 0;

    return (void *)jinue_syscall(&args);
}

int jinue_create_thread(void (*entry)(), void *stack, int *perrno) {
    jinue_syscall_args_t args;

    args.arg0 = JINUE_SYS_CREATE_THREAD;
    args.arg1 = (uintptr_t)entry;
    args.arg2 = (uintptr_t)stack;
    args.arg3 = 0;

    return jinue_syscall_with_usual_convention(&args, perrno);
}

void jinue_yield_thread(void) {
    jinue_syscall_args_t args;

    args.arg0 = JINUE_SYS_YIELD_THREAD;
    args.arg1 = 0;
    args.arg2 = 0;
    args.arg3 = 0;

    jinue_syscall(&args);
}

void jinue_exit_thread(void) {
    jinue_syscall_args_t args;

    args.arg0 = JINUE_SYS_EXIT_THREAD;
    args.arg1 = 0;
    args.arg2 = 0;
    args.arg3 = 0;

    jinue_syscall(&args);
}

void jinue_puts(const char *str, size_t n) {
    jinue_syscall_args_t args;

    args.arg0 = JINUE_SYS_PUTS;
    args.arg1 = (uintptr_t)str;
    args.arg2 = n;
    args.arg3 = 0;

    jinue_syscall(&args);
}
