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

#include <jinue-common/errno.h>
#include <hal/cpu_data.h>
#include <hal/memory.h>
#include <hal/thread.h>
#include <hal/trap.h>
#include <console.h>
#include <ipc.h>
#include <object.h>
#include <printk.h>
#include <process.h>
#include <stddef.h>
#include <stdint.h>
#include <syscall.h>
#include <thread.h>

static void set_return_value_or_error(jinue_syscall_args_t *args, int retval) {
    if(retval < 0) {
        syscall_args_set_error(args, -retval);
    }
    else {
        syscall_args_set_return(args, retval);
    }
}

static void sys_nosys(jinue_syscall_args_t *args) {
    syscall_args_set_error(args, JINUE_ENOSYS);
}

static void sys_get_syscall_method(jinue_syscall_args_t *args) {
    syscall_args_set_return(args, syscall_method);
}

static void sys_console_putc(jinue_syscall_args_t *args) {
    /** TODO: permission check */
    console_putc(
            (char)args->arg1,
            CONSOLE_DEFAULT_COLOR);
    syscall_args_set_return(args, 0);
}

static void sys_console_puts(jinue_syscall_args_t *args) {
    /** TODO: permission check, sanity check (data size vs buffer size) */
    console_printn(
            (char *)args->arg2,
            jinue_args_get_data_size(args),
            CONSOLE_DEFAULT_COLOR);
    syscall_args_set_return(args, 0);
}

static void sys_thread_create(jinue_syscall_args_t *args) {
    void *entry         = (void *)args->arg2;
    void *user_stack    = (void *)args->arg3;

    thread_t *thread = thread_create(
            /* TODO use arg1 as an address space reference if specified */
            get_current_thread()->process,
            entry,
            user_stack);

    /** TODO return descriptor that represents thread */
    if(thread == NULL) {
        syscall_args_set_error(args, JINUE_EAGAIN);
    }
    else {
        syscall_args_set_return(args, 0);
    }
}

static void sys_thread_yield(jinue_syscall_args_t *args) {
    /* TODO this system call yields or destroy depending on arg1. This shouldn't
     * be done by the same system call. */
    thread_yield_from(
            get_current_thread(),
            false,          /* don't block */
            args->arg1);    /* destroy (aka. exit) thread if true */
    syscall_args_set_return(args, 0);
}

static void sys_set_thread_local_address(jinue_syscall_args_t *args) {
    thread_context_set_local_storage(
            &get_current_thread()->thread_ctx,
            (addr_t)args->arg1,
            (size_t)args->arg2);
    syscall_args_set_return(args, 0);
}

static void sys_get_thread_local_address(jinue_syscall_args_t *args) {
    addr_t tls = thread_context_get_local_storage(&get_current_thread()->thread_ctx);
    syscall_args_set_return_ptr(args, tls);
}

static void sys_get_user_memory(jinue_syscall_args_t *args) {
    /** TODO: check user pointer */
    size_t buffer_size      = jinue_args_get_buffer_size(args);
    jinue_mem_map_t *map    = (jinue_mem_map_t *)jinue_args_get_buffer_ptr(args);

    int retval = memory_get_map(map, buffer_size);
    set_return_value_or_error(args, retval);
}

static void sys_create_ipc_endpoint(jinue_syscall_args_t *args) {
    int fd = ipc_create_for_current_process((int)args->arg1);
    set_return_value_or_error(args, fd);
}

static void sys_send(jinue_syscall_args_t *args) {
    ipc_send(args);
}

static void sys_receive(jinue_syscall_args_t *args) {
    ipc_receive(args);
}

static void sys_reply(jinue_syscall_args_t *args) {
    ipc_reply(args);
}

void dispatch_syscall(trapframe_t *trapframe) {
    jinue_syscall_args_t *args = (jinue_syscall_args_t *)&trapframe->msg_arg0;
    
    uintptr_t function_number = args->arg0;
    
    if(function_number < SYSCALL_USER_BASE) {
        /* microkernel system calls */
        switch(function_number) {
        case SYSCALL_FUNC_GET_SYSCALL_METHOD:
            sys_get_syscall_method(args);
            break;
        case SYSCALL_FUNC_CONSOLE_PUTC:
            sys_console_putc(args);
            break;
        case SYSCALL_FUNC_CONSOLE_PUTS:
            sys_console_puts(args);
            break;
        case SYSCALL_FUNC_THREAD_CREATE:
            sys_thread_create(args);
            break;
        case SYSCALL_FUNC_THREAD_YIELD:
            sys_thread_yield(args);
            break;
        case SYSCALL_FUNC_SET_THREAD_LOCAL_ADDR:
            sys_set_thread_local_address(args);
            break;
        case SYSCALL_FUNC_GET_THREAD_LOCAL_ADDR:
            sys_get_thread_local_address(args);
            break;
        case SYSCALL_FUNC_GET_USER_MEMORY:
            sys_get_user_memory(args);
            break;
        case SYSCALL_FUNC_CREATE_IPC_ENDPOINT:
            sys_create_ipc_endpoint(args);
            break;
        case SYSCALL_FUNC_RECEIVE:
            sys_receive(args);
            break;
        case SYSCALL_FUNC_REPLY:
            sys_reply(args);
            break;
        default:
            sys_nosys(args);
        }
    }
    else {
        /* inter-process message */
        sys_send(args);
    }
}
