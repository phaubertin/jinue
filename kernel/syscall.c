/*
 * Copyright (C) 2019-2022 Philippe Aubertin.
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

#include <jinue/shared/errno.h>
#include <jinue/shared/vm.h>
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
#include <string.h>
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

static void sys_get_syscall(jinue_syscall_args_t *args) {
    syscall_args_set_return(args, syscall_method);
}

static void sys_putc(jinue_syscall_args_t *args) {
    /** TODO: permission check */
    console_putc(
            args->arg1 & 0xff,
            CONSOLE_DEFAULT_COLOR);
    syscall_args_set_return(args, 0);
}

static void sys_puts(jinue_syscall_args_t *args) {
    /** TODO: permission check, sanity check on length */
    console_printn(
            (const char *)args->arg1,
            args->arg2,
            CONSOLE_DEFAULT_COLOR);
    syscall_args_set_return(args, 0);
}

static void sys_create_thread(jinue_syscall_args_t *args) {
    void *entry         = (void *)args->arg2;
    void *user_stack    = (void *)args->arg3;

    if(!is_userspace_pointer(entry)) {
        syscall_args_set_error(args, JINUE_EINVAL);
        return;
    }

    if(!is_userspace_pointer(user_stack)) {
        syscall_args_set_error(args, JINUE_EINVAL);
        return;
    }

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

static void sys_yield_thread(jinue_syscall_args_t *args) {
    thread_yield();
    syscall_args_set_return(args, 0);
}

static void sys_exit_thread(jinue_syscall_args_t *args) {
    thread_exit();
    syscall_args_set_return(args, 0);
}

static void sys_set_thread_local(jinue_syscall_args_t *args) {
    addr_t addr = (addr_t)args->arg1;
    size_t size = (size_t)args->arg2;

    if(! check_userspace_buffer(addr, size)) {
        syscall_args_set_error(args, JINUE_EINVAL);
        return;
    }

    thread_set_local_storage(get_current_thread(), addr, size);
    syscall_args_set_return(args, 0);
}

static void sys_get_thread_local(jinue_syscall_args_t *args) {
    addr_t tls = thread_get_local_storage(get_current_thread());
    syscall_args_set_return_ptr(args, tls);
}

static void sys_get_user_memory(jinue_syscall_args_t *args) {
    jinue_buffer_t buffer;

    buffer.addr     = (void *)args->arg1;
    buffer.size     = args->arg2;

    if(! check_userspace_buffer(buffer.addr, buffer.size)) {
        syscall_args_set_error(args, JINUE_EINVAL);
        return;
    }

    int retval = memory_get_map(&buffer);
    set_return_value_or_error(args, retval);
}

static void sys_create_ipc(jinue_syscall_args_t *args) {
    int fd = ipc_create_for_current_process((int)args->arg1);
    set_return_value_or_error(args, fd);
}

static void sys_send(jinue_syscall_args_t *args) {
    jinue_message_t message;

    int function        = args->arg0;
    int fd              = args->arg1;
    void *user_message  = (void *)args->arg2;

    if(! check_userspace_buffer(user_message, sizeof(jinue_message_t))) {
        syscall_args_set_error(args, JINUE_EINVAL);
        return;
    }

    /* Let's be careful here: we need to first copy the message structure and
     * then check it to prevent the user application from modifying the content
     * after we check. */
    memcpy(&message, user_message, sizeof(jinue_message_t));

    size_t send_buffers_size = message.send_buffers_length * sizeof(jinue_buffer_t);
    size_t recv_buffers_size = message.recv_buffers_length * sizeof(jinue_buffer_t);

    if(! check_userspace_buffer(message.send_buffers, send_buffers_size)) {
        syscall_args_set_error(args, JINUE_EINVAL);
        return;
    }

    if(! check_userspace_buffer(message.recv_buffers, recv_buffers_size)) {
        syscall_args_set_error(args, JINUE_EINVAL);
        return;
    }

    int retval = ipc_send(fd, function, &message);
    set_return_value_or_error(args, retval);
}

static void sys_receive(jinue_syscall_args_t *args) {
    jinue_message_t message;

    int fd                          = args->arg1;
    jinue_message_t *user_message   = (jinue_message_t *)args->arg2;

    if(! check_userspace_buffer(user_message, sizeof(jinue_message_t))) {
        syscall_args_set_error(args, JINUE_EINVAL);
        return;
    }

    /* Let's be careful here: we need to first copy the message structure and
     * then check it to prevent the user application from modifying the content
     * after we check. */
    memcpy(&message, user_message, sizeof(jinue_message_t));

    size_t recv_buffers_size = message.recv_buffers_length * sizeof(jinue_buffer_t);

    if(! check_userspace_buffer(message.recv_buffers, recv_buffers_size)) {
        syscall_args_set_error(args, JINUE_EINVAL);
        return;
    }

    int retval = ipc_receive(fd, &message);
    set_return_value_or_error(args, retval);

    if(retval >= 0) {
        user_message->recv_function     = message.recv_function;
        user_message->recv_cookie       = message.recv_cookie;
        user_message->reply_max_size    = message.reply_max_size;
    }
}

static void sys_reply(jinue_syscall_args_t *args) {
    jinue_message_t message;

    void *user_message  = (void *)args->arg2;

    if(! check_userspace_buffer(user_message, sizeof(jinue_message_t))) {
        syscall_args_set_error(args, JINUE_EINVAL);
        return;
    }

    /* Let's be careful here: we need to first copy the message structure and
     * then check it to prevent the user application from modifying the content
     * after we check. */
    memcpy(&message, user_message, sizeof(jinue_message_t));

    size_t send_buffers_size = message.send_buffers_length * sizeof(jinue_buffer_t);

    if(! check_userspace_buffer(message.send_buffers, send_buffers_size)) {
        syscall_args_set_error(args, JINUE_EINVAL);
        return;
    }

    int retval = ipc_reply(&message);
    set_return_value_or_error(args, retval);
}

void dispatch_syscall(trapframe_t *trapframe) {
    jinue_syscall_args_t *args = (jinue_syscall_args_t *)&trapframe->msg_arg0;
    
    intptr_t function = args->arg0;
    
    if(function < 0) {
        /* The function number is expected to be non-negative. This is especially
         * important for the return value of the ipc_receive() system call because,
         * when the system call returns, a negative value (specifically -1), means
         * the call failed. */
        syscall_args_set_error(args, JINUE_EINVAL);
    }
    else if(function < SYSCALL_USER_BASE) {
        /* microkernel system calls */
        switch(function) {
        case SYSCALL_FUNC_GET_SYSCALL:
            sys_get_syscall(args);
            break;
        case SYSCALL_FUNC_PUTC:
            sys_putc(args);
            break;
        case SYSCALL_FUNC_PUTS:
            sys_puts(args);
            break;
        case SYSCALL_FUNC_CREATE_THREAD:
            sys_create_thread(args);
            break;
        case SYSCALL_FUNC_YIELD_THREAD:
            sys_yield_thread(args);
            break;
        case SYSCALL_FUNC_SET_THREAD_LOCAL:
            sys_set_thread_local(args);
            break;
        case SYSCALL_FUNC_GET_THREAD_LOCAL:
            sys_get_thread_local(args);
            break;
        case SYSCALL_FUNC_GET_USER_MEMORY:
            sys_get_user_memory(args);
            break;
        case SYSCALL_FUNC_CREATE_IPC:
            sys_create_ipc(args);
            break;
        case SYSCALL_FUNC_RECEIVE:
            sys_receive(args);
            break;
        case SYSCALL_FUNC_REPLY:
            sys_reply(args);
            break;
        case SYSCALL_FUNC_EXIT_THREAD:
            sys_exit_thread(args);
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
