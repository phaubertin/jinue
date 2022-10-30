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
#include <hal/vga.h>    /* TODO remove this */
#include <console.h>
#include <ipc.h>
#include <limits.h>
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

static int get_descriptor(uintptr_t value) {
    /* This handles the obvious case where the original value was positive and
     * too large, but also the case where an originally negative value was cast
     * to uintptr_t. */
    if(value > INT_MAX) {
        return -JINUE_EBADF;
    }

    return (int)value;
}

static void sys_nosys(jinue_syscall_args_t *args) {
    syscall_args_set_error(args, JINUE_ENOSYS);
}

static void sys_puts(jinue_syscall_args_t *args) {
    int loglevel        = args->arg1;
    const char *message = (const char *)args->arg2;
    size_t length       = args->arg3;

    /* TODO this should be a constant */
    if(length > 120) {
        syscall_args_set_error(args, JINUE_EINVAL);
        return;
    }

    int colour;

    /* TODO move this into VGA code */
    switch(loglevel) {
    case 'I':
        colour = VGA_COLOR_BRIGHTGREEN;
        break;
    case 'W':
        colour = VGA_COLOR_YELLOW;
        break;
    case 'E':
        colour = VGA_COLOR_RED;
        break;
    default:
        syscall_args_set_error(args, JINUE_EINVAL);
        return;
    }

    console_printn(message, length,colour);
    console_putc('\n', colour);
    syscall_args_set_return(args, 0);
}

static void sys_create_thread(jinue_syscall_args_t *args) {
    void *entry         = (void *)args->arg1;
    void *user_stack    = (void *)args->arg2;

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

static int copy_message_struct_from_userspace(
        jinue_message_t         *message,
        const jinue_message_t   *userspace_message) {

    if(! check_userspace_buffer(userspace_message, sizeof(jinue_message_t))) {
        return -JINUE_EINVAL;
    }

    message->send_buffers           = userspace_message->send_buffers;
    message->send_buffers_length    = userspace_message->send_buffers_length;
    message->recv_buffers           = userspace_message->recv_buffers;
    message->recv_buffers_length    = userspace_message->recv_buffers_length;

    return 0;
}

static int check_send_buffers(const jinue_message_t *message) {
    size_t send_buffers_size = message->send_buffers_length * sizeof(jinue_buffer_t);

    if(! check_userspace_buffer(message->send_buffers, send_buffers_size)) {
        return -JINUE_EINVAL;
    }

    return 0;
}

static int check_recv_buffers(const jinue_message_t *message) {
    size_t recv_buffers_size = message->recv_buffers_length * sizeof(jinue_buffer_t);

    if(! check_userspace_buffer(message->recv_buffers, recv_buffers_size)) {
        return -JINUE_EINVAL;
    }

    return 0;
}

static void sys_send(jinue_syscall_args_t *args) {
    jinue_message_t message;

    int function        = args->arg0;
    int fd              = get_descriptor(args->arg1);
    void *user_message  = (void *)args->arg2;

    if(fd < 0) {
        set_return_value_or_error(args, fd);
        return;
    }

    /* Let's be careful here: we need to first copy the message structure and
     * then check it to protect against the user application modifying the
     * content after the check. */
    int copy_retval = copy_message_struct_from_userspace(&message, user_message);

    if(copy_retval < 0) {
        set_return_value_or_error(args, copy_retval);
        return;
    }

    int send_checkval = check_send_buffers(&message);

    if(send_checkval < 0) {
        set_return_value_or_error(args, send_checkval);
        return;
    }

    int recv_checkval = check_recv_buffers(&message);

    if(recv_checkval < 0) {
        set_return_value_or_error(args, recv_checkval);
        return;
    }

    int retval = ipc_send(fd, function, &message);
    set_return_value_or_error(args, retval);
}

static void sys_receive(jinue_syscall_args_t *args) {
    jinue_message_t message;

    int fd                          = get_descriptor(args->arg1);
    jinue_message_t *user_message   = (jinue_message_t *)args->arg2;

    if(fd < 0) {
        set_return_value_or_error(args, fd);
        return;
    }

    /* Let's be careful here: we need to first copy the message structure and
     * then check it to protect against the user application modifying the
     * content after the check. */
    int copy_retval = copy_message_struct_from_userspace(&message, user_message);

    if(copy_retval < 0) {
        set_return_value_or_error(args, copy_retval);
        return;
    }

    int recv_checkval = check_recv_buffers(&message);

    if(recv_checkval < 0) {
        set_return_value_or_error(args, recv_checkval);
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

    /* Let's be careful here: we need to first copy the message structure and
     * then check it to protect against the user application modifying the
     * content after the check. */
    int copy_retval = copy_message_struct_from_userspace(&message, user_message);

    if(copy_retval < 0) {
        set_return_value_or_error(args, copy_retval);
        return;
    }

    int send_checkval = check_send_buffers(&message);

    if(send_checkval < 0) {
        set_return_value_or_error(args, send_checkval);
        return;
    }

    int retval = ipc_reply(&message);
    set_return_value_or_error(args, retval);
}

/**
 * System call dispatching function
 *
 * Dispatch system calls based on the function number present in the call
 * arguments.
 *
 * @param trapframe trap frame for current system call
 *
 */
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
