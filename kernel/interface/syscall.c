/*
 * Copyright (C) 2019-2023 Philippe Aubertin.
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

#include <jinue/shared/asm/errno.h>
#include <jinue/shared/vm.h>
#include <kernel/domain/entities/descriptor.h>
#include <kernel/domain/entities/endpoint.h>
#include <kernel/domain/entities/object.h>
#include <kernel/domain/entities/process.h>
#include <kernel/domain/entities/thread.h>
#include <kernel/domain/services/ipc.h>
#include <kernel/domain/services/logging.h>
#include <kernel/domain/syscalls.h>
#include <kernel/interface/syscall.h>
#include <kernel/utils/utils.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#define ALL_PROT_FLAGS  (JINUE_PROT_READ | JINUE_PROT_WRITE | JINUE_PROT_EXEC)

#define WRITE_EXEC      (JINUE_PROT_WRITE | JINUE_PROT_EXEC)

static void set_return_uintptr(jinue_syscall_args_t *args, uintptr_t retval) {
	args->arg0	= retval;
	args->arg1	= 0;
	args->arg2	= 0;
	args->arg3	= 0;
}

static void set_error(jinue_syscall_args_t *args, int error) {
	args->arg0	= (uintptr_t)-1;
	args->arg1	= (uintptr_t)error;
	args->arg2	= 0;
	args->arg3	= 0;
}

static void set_return_value(jinue_syscall_args_t *args, int retval) {
	set_return_uintptr(args, (uintptr_t)retval);
}

static  void set_return_ptr(jinue_syscall_args_t *args, void *retval) {
	set_return_uintptr(args, (uintptr_t)retval);
}

static void set_return_value_or_error(jinue_syscall_args_t *args, int retval) {
    if(retval < 0) {
        set_error(args, -retval);
    }
    else {
        set_return_value(args, retval);
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
    set_error(args, JINUE_ENOSYS);
}

static void sys_reboot(jinue_syscall_args_t *args) {
    reboot();
}

static void sys_puts(jinue_syscall_args_t *args) {
    int loglevel        = args->arg1;
    const char *message = (const char *)args->arg2;
    size_t length       = args->arg3;

    if(length > JINUE_PUTS_MAX_LENGTH) {
        set_error(args, JINUE_EINVAL);
        return;
    }

    switch(loglevel) {
    case JINUE_PUTS_LOGLEVEL_INFO:
    case JINUE_PUTS_LOGLEVEL_WARNING:
    case JINUE_PUTS_LOGLEVEL_ERROR:
        break;
    default:
        set_error(args, JINUE_EINVAL);
        return;
    }

    logging_add_message(loglevel, message, length);
    set_return_value(args, 0);
}

static void sys_create_thread(jinue_syscall_args_t *args) {
    int process_fd      = get_descriptor(args->arg1);
    void *entry         = (void *)args->arg2;
    void *user_stack    = (void *)args->arg3;

    if(process_fd < 0) {
        set_return_value_or_error(args, process_fd);
        return;
    }

    if(!is_userspace_pointer(entry)) {
        set_error(args, JINUE_EINVAL);
        return;
    }

    if(!is_userspace_pointer(user_stack)) {
        set_error(args, JINUE_EINVAL);
        return;
    }

    /** TODO return descriptor that represents thread */
    int retval = create_thread(process_fd, entry, user_stack);
    set_return_value_or_error(args, retval);
}

static void sys_yield_thread(jinue_syscall_args_t *args) {
    thread_yield();
    set_return_value(args, 0);
}

static void sys_exit_thread(jinue_syscall_args_t *args) {
    thread_exit();
    set_return_value(args, 0);
}

static void sys_set_thread_local(jinue_syscall_args_t *args) {
    addr_t addr = (addr_t)args->arg1;
    size_t size = (size_t)args->arg2;

    if(! check_userspace_buffer(addr, size)) {
        set_error(args, JINUE_EINVAL);
        return;
    }

    set_thread_local(addr, size);
    set_return_value(args, 0);
}

static void sys_get_thread_local(jinue_syscall_args_t *args) {
    void *tls = get_thread_local();
    set_return_ptr(args, tls);
}

static void sys_get_user_memory(jinue_syscall_args_t *args) {
    jinue_buffer_t buffer;

    buffer.addr     = (void *)args->arg1;
    buffer.size     = args->arg2;

    if(! check_userspace_buffer(buffer.addr, buffer.size)) {
        set_error(args, JINUE_EINVAL);
        return;
    }

    int retval = get_user_memory(&buffer);
    set_return_value_or_error(args, retval);
}

static void sys_create_endpoint(jinue_syscall_args_t *args) {
    int fd = get_descriptor(args->arg1);

    if(fd < 0) {
        set_return_value_or_error(args, fd);
        return;  
    }

    int retval = create_endpoint(fd);
    set_return_value_or_error(args, retval);
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
    int function            = args->arg0;
    int fd                  = get_descriptor(args->arg1);
    void *userspace_message = (void *)args->arg2;

    if(fd < 0) {
        set_return_value_or_error(args, fd);
        return;
    }

    /* Let's be careful here: we need to first copy the message structure and
     * then check it to protect against the user application modifying the
     * content after the check. */
    jinue_message_t message;
    int copy_retval = copy_message_struct_from_userspace(&message, userspace_message);

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
    int fd                              = get_descriptor(args->arg1);
    jinue_message_t *userspace_message  = (jinue_message_t *)args->arg2;

    if(fd < 0) {
        set_return_value_or_error(args, fd);
        return;
    }

    /* Let's be careful here: we need to first copy the message structure and
     * then check it to protect against the user application modifying the
     * content after the check. */
    jinue_message_t message;
    int copy_retval = copy_message_struct_from_userspace(&message, userspace_message);

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
        userspace_message->recv_function    = message.recv_function;
        userspace_message->recv_cookie      = message.recv_cookie;
        userspace_message->reply_max_size   = message.reply_max_size;
    }
}

static void sys_reply(jinue_syscall_args_t *args) {
    void *userspace_message = (void *)args->arg2;

    /* Let's be careful here: we need to first copy the message structure and
     * then check it to protect against the user application modifying the
     * content after the check. */
    jinue_message_t message;
    int copy_retval = copy_message_struct_from_userspace(&message, userspace_message);

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

static void sys_mmap(jinue_syscall_args_t *args) {
    const jinue_mmap_args_t *userspace_mmap_args;

    int process_fd      = get_descriptor(args->arg1);
    userspace_mmap_args = (void *)args->arg2;

    if(process_fd < 0) {
        set_return_value_or_error(args, process_fd);
        return;
    }

    if(! check_userspace_buffer(userspace_mmap_args, sizeof(jinue_mmap_args_t))) {
        set_error(args, JINUE_EINVAL);
        return;
    }

    jinue_mmap_args_t mmap_args = *userspace_mmap_args;

    if(OFFSET_OF_PTR(mmap_args.addr, PAGE_SIZE) != 0) {
        set_error(args, JINUE_EINVAL);
        return;
    }

    if((mmap_args.length & (PAGE_SIZE -1)) != 0) {
        set_error(args, JINUE_EINVAL);
        return;
    }

    if((mmap_args.paddr & (PAGE_SIZE -1)) != 0) {
        set_error(args, JINUE_EINVAL);
        return;
    }

    if((mmap_args.prot & ~ALL_PROT_FLAGS) != 0) {
        set_error(args, JINUE_EINVAL);
        return;
    }

    if((mmap_args.prot & WRITE_EXEC) == WRITE_EXEC) {
        set_error(args, JINUE_ENOTSUP);
        return;
    }

    int retval = mmap(process_fd, &mmap_args);
    set_return_value_or_error(args, retval);
}

static void sys_create_process(jinue_syscall_args_t *args) {
    int fd = get_descriptor(args->arg1);

    if(fd < 0) {
        set_return_value_or_error(args, fd);
        return;
    }

    int retval = create_process(fd);
    set_return_value_or_error(args, retval);
}

static void sys_mclone(jinue_syscall_args_t *args) {
    const jinue_mclone_args_t *userspace_mclone_args;

    int src                 = get_descriptor(args->arg1);
    int dest                = get_descriptor(args->arg2);
    userspace_mclone_args   = (void *)args->arg3;

    if(src < 0) {
        set_return_value_or_error(args, src);
        return;
    }

    if(dest < 0) {
        set_return_value_or_error(args, dest);
        return;
    }

    if(! check_userspace_buffer(userspace_mclone_args, sizeof(jinue_mclone_args_t))) {
        set_error(args, JINUE_EINVAL);
        return;
    }

    jinue_mclone_args_t mclone_args = *userspace_mclone_args;

    if(OFFSET_OF_PTR(mclone_args.src_addr, PAGE_SIZE) != 0) {
        set_error(args, JINUE_EINVAL);
        return;
    }

    if(OFFSET_OF_PTR(mclone_args.dest_addr, PAGE_SIZE) != 0) {
        set_error(args, JINUE_EINVAL);
        return;
    }

    if((mclone_args.length & (PAGE_SIZE -1)) != 0) {
        set_error(args, JINUE_EINVAL);
        return;
    }

    if((mclone_args.prot & ~ALL_PROT_FLAGS) != 0) {
        set_error(args, JINUE_EINVAL);
        return;
    }

    if((mclone_args.prot & WRITE_EXEC) == WRITE_EXEC) {
        set_error(args, JINUE_ENOTSUP);
        return;
    }

    int retval = mclone(src, dest, &mclone_args);
    set_return_value_or_error(args, retval);
}

static void sys_dup(jinue_syscall_args_t *args) {
    int process_fd  = get_descriptor(args->arg1);
    int src         = get_descriptor(args->arg2);
    int dest        = get_descriptor(args->arg3);

    if(process_fd < 0) {
        set_return_value_or_error(args, process_fd);
        return;
    }

    if(src < 0) {
        set_return_value_or_error(args, src);
        return;
    }

    if(dest < 0) {
        set_return_value_or_error(args, dest);
        return;
    }

    int retval = dup(process_fd, src, dest);
    set_return_value_or_error(args, retval);
}

static void sys_close(jinue_syscall_args_t *args) {
    int fd = get_descriptor(args->arg1);

    if(fd < 0) {
        set_return_value_or_error(args, fd);
        return;
    }

    int retval = close(fd);
    set_return_value_or_error(args, retval);
}

static void sys_destroy(jinue_syscall_args_t *args) {
    int fd = get_descriptor(args->arg1);

    if(fd < 0) {
        set_return_value_or_error(args, fd);
        return;
    }

    int retval = destroy(fd);
    set_return_value_or_error(args, retval);
}

static void sys_mint(jinue_syscall_args_t *args) {
    const jinue_mint_args_t *userspace_mint_args;
    int owner           = get_descriptor(args->arg1);
    userspace_mint_args = (void *)args->arg2;

    if(owner < 0) {
        set_return_value_or_error(args, owner);
        return;
    }

    if(! check_userspace_buffer(userspace_mint_args, sizeof(jinue_mint_args_t))) {
        set_error(args, JINUE_EINVAL);
        return;
    }

    jinue_mint_args_t mint_args;
    mint_args.process   = get_descriptor(userspace_mint_args->process);
    mint_args.fd        = get_descriptor(userspace_mint_args->fd);
    mint_args.perms     = userspace_mint_args->perms;
    mint_args.cookie    = userspace_mint_args->cookie;
    
    if(mint_args.process < 0) {
        set_return_value_or_error(args, mint_args.process);
        return;
    }

    if(mint_args.fd < 0) {
        set_return_value_or_error(args, mint_args.fd);
        return;
    }

    int retval = mint(owner, &mint_args);
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
void dispatch_syscall(jinue_syscall_args_t *args) {
    intptr_t function = args->arg0;
    
    if(function < 0) {
        /* The function number is expected to be non-negative. This is especially
         * important for the return value of the ipc_receive() system call because,
         * when the system call returns, a negative value (specifically -1), means
         * the call failed. */
        set_error(args, JINUE_EINVAL);
    }
    else if(function < JINUE_SYS_USER_BASE) {
        /* microkernel system calls */
        switch(function) {
        case JINUE_SYS_REBOOT:
            sys_reboot(args);
            break;
        case JINUE_SYS_PUTS:
            sys_puts(args);
            break;
        case JINUE_SYS_CREATE_THREAD:
            sys_create_thread(args);
            break;
        case JINUE_SYS_YIELD_THREAD:
            sys_yield_thread(args);
            break;
        case JINUE_SYS_SET_THREAD_LOCAL:
            sys_set_thread_local(args);
            break;
        case JINUE_SYS_GET_THREAD_LOCAL:
            sys_get_thread_local(args);
            break;
        case JINUE_SYS_GET_USER_MEMORY:
            sys_get_user_memory(args);
            break;
        case JINUE_SYS_CREATE_ENDPOINT:
            sys_create_endpoint(args);
            break;
        case JINUE_SYS_RECEIVE:
            sys_receive(args);
            break;
        case JINUE_SYS_REPLY:
            sys_reply(args);
            break;
        case JINUE_SYS_EXIT_THREAD:
            sys_exit_thread(args);
            break;
        case JINUE_SYS_MMAP:
            sys_mmap(args);
            break;
        case JINUE_SYS_CREATE_PROCESS:
            sys_create_process(args);
            break;
        case JINUE_SYS_MCLONE:
            sys_mclone(args);
            break;
        case JINUE_SYS_DUP:
            sys_dup(args);
            break;
        case JINUE_SYS_CLOSE:
            sys_close(args);
            break;
        case JINUE_SYS_DESTROY:
            sys_destroy(args);
            break;
        case JINUE_SYS_MINT:
            sys_mint(args);
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
