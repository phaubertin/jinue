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


typedef void (*syscall_stub_t)(jinue_syscall_args_t *args);

/* these are defined in stubs.asm */

void syscall_fast_intel(jinue_syscall_args_t *args);

void syscall_fast_amd(jinue_syscall_args_t *args);

void syscall_intr(jinue_syscall_args_t *args);


static syscall_stub_t syscall_stubs[] = {
        [SYSCALL_METHOD_FAST_INTEL] = syscall_fast_intel,
        [SYSCALL_METHOD_FAST_AMD]   = syscall_fast_amd,
        [SYSCALL_METHOD_INTR]       = syscall_intr
};

static char *syscall_stub_names[] = {
        [SYSCALL_METHOD_FAST_INTEL] = "SYSENTER/SYSEXIT (fast Intel)",
        [SYSCALL_METHOD_FAST_AMD]   = "SYSCALL/SYSRET (fast AMD)",
        [SYSCALL_METHOD_INTR]       = "interrupt",
};

static int syscall_stub_index           = SYSCALL_METHOD_INTR;

static syscall_stub_t syscall_stub_ptr  = syscall_intr;


void jinue_call_raw(jinue_syscall_args_t *args) {
    syscall_stub_ptr(args);
}

int jinue_call(jinue_syscall_args_t *args, int *perrno) {
    jinue_call_raw(args);

    if(perrno != NULL) {
        if(args->arg1 < 0) {
            *perrno = args->arg1;
        }
    }

    return (int)(args->arg0);
}

void jinue_get_syscall_implementation(void) {
    jinue_syscall_args_t args;

    args.arg0 = SYSCALL_FUNC_GET_SYSCALL_METHOD;
    args.arg1 = 0;
    args.arg2 = 0;
    args.arg3 = 0;

    jinue_call_raw(&args);

    syscall_stub_index  = jinue_get_return(&args);
    syscall_stub_ptr    = syscall_stubs[syscall_stub_index];
}

const char *jinue_get_syscall_implementation_name(void) {
    return syscall_stub_names[syscall_stub_index];
}

void jinue_set_thread_local_storage(void *addr, size_t size) {
    jinue_syscall_args_t args;

    args.arg0 = SYSCALL_FUNC_SET_THREAD_LOCAL_ADDR;
    args.arg1 = (uintptr_t)addr;
    args.arg2 = (uintptr_t)size;
    args.arg3 = 0;

    (void)jinue_call(&args, NULL);
}

void *jinue_get_thread_local_storage(void) {
    jinue_syscall_args_t args;

    args.arg0 = SYSCALL_FUNC_GET_THREAD_LOCAL_ADDR;
    args.arg1 = 0;
    args.arg2 = 0;
    args.arg3 = 0;

    (void)jinue_call(&args, NULL);

    return (void *)jinue_get_return_uintptr(&args);
}

int jinue_thread_create(void (*entry)(), void *stack, int *perrno) {
    jinue_syscall_args_t args;

    args.arg0 = SYSCALL_FUNC_THREAD_CREATE;
    args.arg1 = 0;
    args.arg2 = (uintptr_t)entry;
    args.arg3 = (uintptr_t)stack;

    return jinue_call(&args, perrno);
}

int jinue_yield(void) {
    jinue_syscall_args_t args;

    args.arg0 = SYSCALL_FUNC_THREAD_YIELD;
    args.arg1 = false;
    args.arg2 = 0;
    args.arg3 = 0;

    return jinue_call(&args, NULL);
}

void jinue_thread_exit(void) {
    jinue_syscall_args_t args;

    args.arg0 = SYSCALL_FUNC_THREAD_YIELD;
    args.arg1 = true;
    args.arg2 = 0;
    args.arg3 = 0;

    (void)jinue_call(&args, NULL);
}
