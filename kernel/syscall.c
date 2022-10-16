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

static int check_input_buffer(
        syscall_input_buffer_t  *buffer,
        jinue_syscall_args_t    *args) {

    buffer->user_ptr    = jinue_args_get_buffer_ptr(args);
    buffer->buffer_size = jinue_args_get_buffer_size(args);
    buffer->data_size   = jinue_args_get_data_size(args);
    buffer->desc_n      = jinue_args_get_n_desc(args);
    /* TODO aren't we missing padding size here? */
    buffer->total_size  =
            buffer->data_size + buffer->desc_n * sizeof(jinue_ipc_descriptor_t);

    if(buffer->buffer_size > JINUE_SEND_MAX_SIZE) {
        return -JINUE_EINVAL;
    }

    if(buffer->total_size > buffer->buffer_size) {
        return -JINUE_EINVAL;
    }

    if(buffer->desc_n > JINUE_SEND_MAX_N_DESC) {
        return -JINUE_EINVAL;
    }

    if(! check_userspace_buffer(buffer->user_ptr, buffer->buffer_size)) {
        return -JINUE_EINVAL;
    }

    return 0;
}

static int check_output_buffer(
        syscall_output_buffer_t *buffer,
        jinue_syscall_args_t    *args) {

    buffer->user_ptr    = jinue_args_get_buffer_ptr(args);
    buffer->buffer_size = jinue_args_get_buffer_size(args);

    if(buffer->buffer_size > JINUE_SEND_MAX_SIZE) {
        return -JINUE_EINVAL;
    }

    if(! check_userspace_buffer(buffer->user_ptr, buffer->buffer_size)) {
        return -JINUE_EINVAL;
    }

    return 0;
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
    syscall_output_buffer_t buffer;

    buffer.user_ptr     = (void *)args->arg1;
    buffer.buffer_size  = args->arg2;

    if(! check_userspace_buffer(buffer.user_ptr, buffer.buffer_size)) {
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
    syscall_input_buffer_t buffer;

    int function    = args->arg0;
    int fd          = args->arg1;
    int checkval    = check_input_buffer(&buffer, args);

    if(checkval < 0) {
        syscall_args_set_error(args, -checkval);
        return;
    }

    /* We need to pass the full args here so the receiver thread can set the
     * return values in ipc_reply(). */
    int retval = ipc_send(fd, function, &buffer, args);

    if(retval < 0) {
        set_return_value_or_error(args, retval);
    }
}

static void sys_receive(jinue_syscall_args_t *args) {
    syscall_output_buffer_t  buffer;

    int fd          = (int)args->arg1;
    int checkval    = check_output_buffer(&buffer, args);

    if(checkval < 0) {
        syscall_args_set_error(args, -checkval);
        return;
    }

    /* This function does not set only a return value on success but needs to
     * be able to set the value of all registers, which is why we pass the
     * full args here. */
    int retval = ipc_receive(fd, &buffer, args);

    /* ipc_receive() sets the return values on success so we only need to
     * handle the error cases here. */
    if(retval < 0) {
        syscall_args_set_error(args, -retval);
    }
}

static void sys_reply(jinue_syscall_args_t *args) {
    syscall_input_buffer_t buffer;

    int checkval = check_input_buffer(&buffer, args);

    if(checkval < 0) {
        syscall_args_set_error(args, -checkval);
        return;
    }

    int retval = ipc_reply(&buffer);
    set_return_value_or_error(args, retval);
}

void dispatch_syscall(trapframe_t *trapframe) {
    jinue_syscall_args_t *args = (jinue_syscall_args_t *)&trapframe->msg_arg0;
    
    int function = args->arg0;
    
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
