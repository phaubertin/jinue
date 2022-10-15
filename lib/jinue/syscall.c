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
        [SYSCALL_METHOD_FAST_INTEL] = jinue_syscall_fast_intel,
        [SYSCALL_METHOD_FAST_AMD]   = jinue_syscall_fast_amd,
        [SYSCALL_METHOD_INTR]       = jinue_syscall_intr
};

static char *syscall_stub_names[] = {
        [SYSCALL_METHOD_FAST_INTEL] = "SYSENTER/SYSEXIT (fast Intel)",
        [SYSCALL_METHOD_FAST_AMD]   = "SYSCALL/SYSRET (fast AMD)",
        [SYSCALL_METHOD_INTR]       = "interrupt",
};

static int syscall_stub_index = SYSCALL_METHOD_INTR;

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

int jinue_get_syscall(void) {
    jinue_syscall_args_t args;

    args.arg0 = SYSCALL_FUNC_GET_SYSCALL;
    args.arg1 = 0;
    args.arg2 = 0;
    args.arg3 = 0;

    syscall_stub_index = (intptr_t)jinue_syscall(&args);

    return syscall_stub_index;
}

const char *jinue_get_syscall_implementation_name(void) {
    return syscall_stub_names[syscall_stub_index];
}

void jinue_set_thread_local(void *addr, size_t size) {
    jinue_syscall_args_t args;

    args.arg0 = SYSCALL_FUNC_SET_THREAD_LOCAL;
    args.arg1 = (uintptr_t)addr;
    args.arg2 = (uintptr_t)size;
    args.arg3 = 0;

    jinue_syscall(&args);
}

void *jinue_get_thread_local(void) {
    jinue_syscall_args_t args;

    args.arg0 = SYSCALL_FUNC_GET_THREAD_LOCAL;
    args.arg1 = 0;
    args.arg2 = 0;
    args.arg3 = 0;

    return (void *)jinue_syscall(&args);
}

int jinue_create_thread(void (*entry)(), void *stack, int *perrno) {
    jinue_syscall_args_t args;

    args.arg0 = SYSCALL_FUNC_CREATE_THREAD;
    args.arg1 = 0;
    args.arg2 = (uintptr_t)entry;
    args.arg3 = (uintptr_t)stack;

    return jinue_syscall_with_usual_convention(&args, perrno);
}

void jinue_yield_thread(void) {
    jinue_syscall_args_t args;

    args.arg0 = SYSCALL_FUNC_YIELD_THREAD;
    args.arg1 = 0;
    args.arg2 = 0;
    args.arg3 = 0;

    jinue_syscall(&args);
}

void jinue_exit_thread(void) {
    jinue_syscall_args_t args;

    args.arg0 = SYSCALL_FUNC_EXIT_THREAD;
    args.arg1 = 0;
    args.arg2 = 0;
    args.arg3 = 0;

    jinue_syscall(&args);
}

void jinue_putc(char c) {
    jinue_syscall_args_t args;

    args.arg0 = SYSCALL_FUNC_PUTC;
    args.arg1 = c & 0xff;
    args.arg2 = 0;
    args.arg3 = 0;

    jinue_syscall(&args);
}

int jinue_puts(const char *str, size_t n, int *perrno) {
    jinue_syscall_args_t args;

    if(n > JINUE_SEND_MAX_SIZE) {
        jinue_set_errno(perrno, JINUE_EINVAL);
        return -1;
    }

    args.arg0 = SYSCALL_FUNC_PUTS;
    args.arg1 = 0;
    args.arg2 = (uintptr_t)str;
    args.arg3 = jinue_args_pack_data_size(n);

    jinue_syscall(&args);

    return 0;
}
