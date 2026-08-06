/*
 * Copyright (C) 2019-2026 Philippe Aubertin.
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
#include <jinue/shared/asm/signal.h>
#include <jinue/shared/asm/syscalls.h>
#include <jinue/shared/asm/mman.h>
#include <jinue/shared/types.h>
#include <kernel/application/syscalls.h>
#include <kernel/domain/entities/descriptor.h>
#include <kernel/domain/entities/endpoint.h>
#include <kernel/domain/entities/object.h>
#include <kernel/domain/entities/process.h>
#include <kernel/interface/machine/signal.h>
#include <kernel/interface/machine/trap.h>
#include <kernel/interface/syscalls.h>
#include <kernel/machine/asm/machine.h>
#include <kernel/utils/utils.h>
#include <kernel/utils/pmap.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#define ALL_PROT_FLAGS  (JINUE_PROT_READ | JINUE_PROT_WRITE | JINUE_PROT_EXEC)

#define WRITE_EXEC      (JINUE_PROT_WRITE | JINUE_PROT_EXEC)

#define ALL_MAP_FLAGS   (JINUE_MAP_UNCACHEABLE | JINUE_MAP_WRITE_COMBINE)

#define UC_WC           (JINUE_MAP_UNCACHEABLE | JINUE_MAP_WRITE_COMBINE)

static void set_return_value(trapframe_t *trapframe, int retval) {
    msg_arg0(trapframe) = (uintptr_t)retval;
    msg_arg1(trapframe) = 0;
    msg_arg2(trapframe) = 0;
    msg_arg3(trapframe) = 0;
}

static void set_error(trapframe_t *trapframe, int error) {
    msg_arg0(trapframe) = (uintptr_t)-1;
    msg_arg1(trapframe) = (uintptr_t)error;
    msg_arg2(trapframe) = 0;
    msg_arg3(trapframe) = 0;
}

static void set_return_value_or_error(trapframe_t *trapframe, int retval) {
    if(retval < 0) {
        set_error(trapframe, -retval);
    }
    else {
        set_return_value(trapframe, retval);
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

static void sys_nosys(trapframe_t *trapframe) {
    set_error(trapframe, JINUE_ENOSYS);
}

static void sys_reboot(trapframe_t *trapframe) {
    reboot();
}

static void sys_puts(trapframe_t *trapframe) {
    uint8_t loglevel    = msg_arg1(trapframe) & 0xff;
    uint8_t facility    = (msg_arg1(trapframe) >> 8) & 0xff;
    const char *str     = (const char *)msg_arg2(trapframe);
    size_t length       = msg_arg3(trapframe);

    int retval = puts(loglevel, facility, str, length);
    set_return_value_or_error(trapframe, retval);
}

static void sys_create_thread(trapframe_t *trapframe) {
    int fd          = get_descriptor(msg_arg1(trapframe));
    int process_fd  = get_descriptor(msg_arg2(trapframe));

    if(fd < 0) {
        set_return_value_or_error(trapframe, fd);
        return;
    }

    if(process_fd < 0) {
        set_return_value_or_error(trapframe, process_fd);
        return;
    }

    int retval = create_thread(fd, process_fd);
    set_return_value_or_error(trapframe, retval);
}

static void sys_yield_thread(trapframe_t *trapframe) {
    yield_thread();
    set_return_value(trapframe, 0);
}

static void sys_exit_thread(trapframe_t *trapframe) {
    exit_thread();
    /* No need to set a return value since exit_thread() does not return. */
}

static void sys_set_thread_local(trapframe_t *trapframe) {
    addr_t addr = (addr_t)msg_arg1(trapframe);
    size_t size = (size_t)msg_arg2(trapframe);

    if(! check_userspace_buffer(addr, size)) {
        set_error(trapframe, JINUE_EINVAL);
        return;
    }

    set_thread_local(addr, size);
    set_return_value(trapframe, 0);
}

static void sys_get_address_map(trapframe_t *trapframe) {
    jinue_buffer_t buffer;

    buffer.addr     = (void *)msg_arg1(trapframe);
    buffer.size     = msg_arg2(trapframe);

    if(! check_userspace_buffer(buffer.addr, buffer.size)) {
        set_error(trapframe, JINUE_EINVAL);
        return;
    }

    int retval = get_address_map(&buffer);
    set_return_value_or_error(trapframe, retval);
}

static void sys_create_endpoint(trapframe_t *trapframe) {
    int fd = get_descriptor(msg_arg1(trapframe));

    if(fd < 0) {
        set_return_value_or_error(trapframe, fd);
        return;  
    }

    int retval = create_endpoint(fd);
    set_return_value_or_error(trapframe, retval);
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

static void sys_send(trapframe_t *trapframe) {
    int function            = msg_arg0(trapframe);
    int fd                  = get_descriptor(msg_arg1(trapframe));
    void *userspace_message = (void *)msg_arg2(trapframe);

    if(fd < 0) {
        set_return_value_or_error(trapframe, fd);
        return;
    }

    /* Let's be careful here: we need to first copy the message structure and
     * then check it to protect against the user application modifying the
     * content after the check. */
    jinue_message_t message;
    int copy_retval = copy_message_struct_from_userspace(&message, userspace_message);

    if(copy_retval < 0) {
        set_return_value_or_error(trapframe, copy_retval);
        return;
    }

    int send_checkval = check_send_buffers(&message);

    if(send_checkval < 0) {
        set_return_value_or_error(trapframe, send_checkval);
        return;
    }

    int recv_checkval = check_recv_buffers(&message);

    if(recv_checkval < 0) {
        set_return_value_or_error(trapframe, recv_checkval);
        return;
    }

    uintptr_t *errcode = &msg_arg2(trapframe);
    int retval = send(errcode, fd, function, &message);

    if(retval == -JINUE_EPROTO) {
        msg_arg0(trapframe) = -1;
        msg_arg1(trapframe) = JINUE_EPROTO;
        /* The error code has already been set in arg2. */
        msg_arg3(trapframe) = 0;
        return;
    }

    set_return_value_or_error(trapframe, retval);
}

static void sys_receive(trapframe_t *trapframe) {
    int fd                              = get_descriptor(msg_arg1(trapframe));
    jinue_message_t *userspace_message  = (jinue_message_t *)msg_arg2(trapframe);

    if(fd < 0) {
        set_return_value_or_error(trapframe, fd);
        return;
    }

    /* Let's be careful here: we need to first copy the message structure and
     * then check it to protect against the user application modifying the
     * content after the check. */
    jinue_message_t message;
    int copy_retval = copy_message_struct_from_userspace(&message, userspace_message);

    if(copy_retval < 0) {
        set_return_value_or_error(trapframe, copy_retval);
        return;
    }

    int recv_checkval = check_recv_buffers(&message);

    if(recv_checkval < 0) {
        set_return_value_or_error(trapframe, recv_checkval);
        return;
    }

    int retval = receive(fd, &message);
    set_return_value_or_error(trapframe, retval);

    if(retval >= 0) {
        userspace_message->recv_function    = message.recv_function;
        userspace_message->recv_cookie      = message.recv_cookie;
        userspace_message->reply_max_size   = message.reply_max_size;
    }
}

static void sys_reply(trapframe_t *trapframe) {
    void *userspace_message = (void *)msg_arg2(trapframe);

    /* Let's be careful here: we need to first copy the message structure and
     * then check it to protect against the user application modifying the
     * content after the check. */
    jinue_message_t message;
    int copy_retval = copy_message_struct_from_userspace(&message, userspace_message);

    if(copy_retval < 0) {
        set_return_value_or_error(trapframe, copy_retval);
        return;
    }

    int send_checkval = check_send_buffers(&message);

    if(send_checkval < 0) {
        set_return_value_or_error(trapframe, send_checkval);
        return;
    }

    int retval = reply(&message);
    set_return_value_or_error(trapframe, retval);
}

static void sys_mmap(trapframe_t *trapframe) {
    const jinue_mmap_args_t *userspace_mmap_args;

    int process_fd      = get_descriptor(msg_arg1(trapframe));
    userspace_mmap_args = (void *)msg_arg2(trapframe);

    if(process_fd < 0) {
        set_return_value_or_error(trapframe, process_fd);
        return;
    }

    if(! check_userspace_buffer(userspace_mmap_args, sizeof(jinue_mmap_args_t))) {
        set_error(trapframe, JINUE_EINVAL);
        return;
    }

    jinue_mmap_args_t mmap_args = *userspace_mmap_args;

    if(OFFSET_OF_PTR(mmap_args.addr, PAGE_SIZE) != 0) {
        set_error(trapframe, JINUE_EINVAL);
        return;
    }

    if((mmap_args.length & (PAGE_SIZE -1)) != 0) {
        set_error(trapframe, JINUE_EINVAL);
        return;
    }

    if((mmap_args.paddr & (PAGE_SIZE -1)) != 0) {
        set_error(trapframe, JINUE_EINVAL);
        return;
    }

    if((mmap_args.prot & ~ALL_PROT_FLAGS) != 0) {
        set_error(trapframe, JINUE_EINVAL);
        return;
    }

    if((mmap_args.prot & WRITE_EXEC) == WRITE_EXEC) {
        set_error(trapframe, JINUE_ENOTSUP);
        return;
    }

    if((mmap_args.flags & ~ALL_MAP_FLAGS) != 0) {
        set_error(trapframe, JINUE_EINVAL);
        return;
    }

    if((mmap_args.flags & UC_WC) == UC_WC) {
        set_error(trapframe, JINUE_ENOTSUP);
        return;
    }

    int retval = mmap(process_fd, &mmap_args);
    set_return_value_or_error(trapframe, retval);
}

static void sys_create_process(trapframe_t *trapframe) {
    int fd = get_descriptor(msg_arg1(trapframe));

    if(fd < 0) {
        set_return_value_or_error(trapframe, fd);
        return;
    }

    int retval = create_process(fd);
    set_return_value_or_error(trapframe, retval);
}

static void sys_dup(trapframe_t *trapframe) {
    int process_fd  = get_descriptor(msg_arg1(trapframe));
    int src         = get_descriptor(msg_arg2(trapframe));
    int dest        = get_descriptor(msg_arg3(trapframe));

    if(process_fd < 0) {
        set_return_value_or_error(trapframe, process_fd);
        return;
    }

    if(src < 0) {
        set_return_value_or_error(trapframe, src);
        return;
    }

    if(dest < 0) {
        set_return_value_or_error(trapframe, dest);
        return;
    }

    int retval = dup(process_fd, src, dest);
    set_return_value_or_error(trapframe, retval);
}

static void sys_close(trapframe_t *trapframe) {
    int fd = get_descriptor(msg_arg1(trapframe));

    if(fd < 0) {
        set_return_value_or_error(trapframe, fd);
        return;
    }

    int retval = close(fd);
    set_return_value_or_error(trapframe, retval);
}

static void sys_destroy(trapframe_t *trapframe) {
    int fd = get_descriptor(msg_arg1(trapframe));

    if(fd < 0) {
        set_return_value_or_error(trapframe, fd);
        return;
    }

    int retval = destroy(fd);
    set_return_value_or_error(trapframe, retval);
}

static void sys_mint(trapframe_t *trapframe) {
    const jinue_mint_args_t *userspace_mint_args;
    int owner           = get_descriptor(msg_arg1(trapframe));
    userspace_mint_args = (void *)msg_arg2(trapframe);

    if(owner < 0) {
        set_return_value_or_error(trapframe, owner);
        return;
    }

    if(! check_userspace_buffer(userspace_mint_args, sizeof(jinue_mint_args_t))) {
        set_error(trapframe, JINUE_EINVAL);
        return;
    }

    jinue_mint_args_t mint_args;
    mint_args.process   = get_descriptor(userspace_mint_args->process);
    mint_args.fd        = get_descriptor(userspace_mint_args->fd);
    mint_args.perms     = userspace_mint_args->perms;
    mint_args.cookie    = userspace_mint_args->cookie;
    
    if(mint_args.process < 0) {
        set_return_value_or_error(trapframe, mint_args.process);
        return;
    }

    if(mint_args.fd < 0) {
        set_return_value_or_error(trapframe, mint_args.fd);
        return;
    }

    int retval = mint(owner, &mint_args);
    set_return_value_or_error(trapframe, retval);
}

static void sys_start_thread(trapframe_t *trapframe) {
    const jinue_start_thread_args_t *userspace_start_args;
    int fd                  = get_descriptor(msg_arg1(trapframe));
    userspace_start_args    = (void *)msg_arg2(trapframe);

    if(fd < 0) {
        set_return_value_or_error(trapframe, fd);
        return;
    }

    if(!check_userspace_buffer(userspace_start_args, sizeof(jinue_start_thread_args_t))) {
        set_error(trapframe, JINUE_EINVAL);
        return;
    }

    void (*entry)(void) = userspace_start_args->entry;
    void *stack_addr= userspace_start_args->stack_addr;
    const jinue_sigset_t *sigset = userspace_start_args->sigset;

    if(!is_userspace_pointer((void *)(uintptr_t)entry)) {
        set_error(trapframe, JINUE_EINVAL);
        return;
    }

    if(!is_userspace_pointer(stack_addr)) {
        set_error(trapframe, JINUE_EINVAL);
        return;
    }

    if(!check_userspace_buffer(sigset, sizeof(jinue_sigset_t))) {
        set_error(trapframe, JINUE_EINVAL);
        return;
    }

    int retval = start_thread(fd, entry, stack_addr, sigset->sa_sigbits[0]);
    set_return_value_or_error(trapframe, retval);
}

static void sys_await_thread(trapframe_t *trapframe) {
    int fd                      = get_descriptor(msg_arg1(trapframe));

    if(fd < 0) {
        set_return_value_or_error(trapframe, fd);
        return;
    }

    int retval = await_thread(fd);
    set_return_value_or_error(trapframe, retval);
}

static void sys_reply_error(trapframe_t *trapframe) {
    uintptr_t errcode   = msg_arg1(trapframe);
    int retval          = reply_error(errcode);
    set_return_value_or_error(trapframe, retval);
}

static void sys_signal_process(trapframe_t *trapframe) {
    uintptr_t arg1  = msg_arg1(trapframe);
    int signo       = msg_arg2(trapframe);

    int fd;

    if((int)arg1 == -1) {
        fd = -1;
    }
    else {
        fd = get_descriptor(arg1);

        if(fd < 0) {
            set_return_value_or_error(trapframe, fd);
            return;
        }
    }

    int retval  = signal_process(fd, signo);
    set_return_value_or_error(trapframe, retval);
}

static void sys_signal_thread(trapframe_t *trapframe) {
    uintptr_t arg1  = msg_arg1(trapframe);
    int signo       = msg_arg2(trapframe);
    
    int fd;

    if((int)arg1 == -1) {
        fd = -1;
    }
    else {
        fd = get_descriptor(arg1);

        if(fd < 0) {
            set_return_value_or_error(trapframe, fd);
            return;
        }
    }
    
    int retval  = signal_thread(fd, signo);
    set_return_value_or_error(trapframe, retval);
}

static void sys_return_from_signal(trapframe_t *trapframe) {
    const jinue_ucontext_t *ucontext = (const jinue_ucontext_t *)msg_arg1(trapframe);

    if(!check_userspace_buffer(ucontext, sizeof(jinue_ucontext_t))) {
        set_error(trapframe, JINUE_EINVAL);
        return;
    }

    int retval  = return_from_signal(trapframe, ucontext);
    set_return_value_or_error(trapframe, retval);
}

static void sys_get_set_signal_mask(trapframe_t *trapframe) {
    int how                     = (int)msg_arg1(trapframe);
    const jinue_sigset_t *set   = (const jinue_sigset_t *)msg_arg2(trapframe);
    jinue_sigset_t *oset        = (jinue_sigset_t *)msg_arg3(trapframe);

    if(set == NULL) {
        how = JINUE_SIG_NONE;
    }

    if(set != NULL && !check_userspace_buffer(set, sizeof(jinue_sigset_t))) {
        set_error(trapframe, JINUE_EINVAL);
        return;
    }

    if(oset != NULL && !check_userspace_buffer(oset, sizeof(jinue_sigset_t))) {
        set_error(trapframe, JINUE_EINVAL);
        return;
    }

    int retval = get_set_signal_mask(how, set != NULL ? set->sa_sigbits[0] : 0, oset);

    if(retval < 0) {
        set_error(trapframe, -retval);
        return;
    }
    
    set_return_value(trapframe, retval);
}

/**
 * System call dispatching function
 *
 * Dispatch system calls based on the function number present in the call
 * arguments.
 *
 * @param trapframe trap frame for current system call
 */
void handle_syscall(trapframe_t *trapframe) {
    intptr_t function = msg_arg0(trapframe);
    
    if(function < 0) {
        set_error(trapframe, JINUE_EINVAL);
    }
    else if(function < JINUE_SYS_USER_BASE) {
        /* microkernel system calls */
        switch(function) {
        case JINUE_SYS_REBOOT:
            sys_reboot(trapframe);
            break;
        case JINUE_SYS_PUTS:
            sys_puts(trapframe);
            break;
        case JINUE_SYS_CREATE_THREAD:
            sys_create_thread(trapframe);
            break;
        case JINUE_SYS_YIELD_THREAD:
            sys_yield_thread(trapframe);
            break;
        case JINUE_SYS_SET_THREAD_LOCAL:
            sys_set_thread_local(trapframe);
            break;
        case JINUE_SYS_GET_ADDRESS_MAP:
            sys_get_address_map(trapframe);
            break;
        case JINUE_SYS_CREATE_ENDPOINT:
            sys_create_endpoint(trapframe);
            break;
        case JINUE_SYS_RECEIVE:
            sys_receive(trapframe);
            break;
        case JINUE_SYS_REPLY:
            sys_reply(trapframe);
            break;
        case JINUE_SYS_EXIT_THREAD:
            sys_exit_thread(trapframe);
            break;
        case JINUE_SYS_MMAP:
            sys_mmap(trapframe);
            break;
        case JINUE_SYS_CREATE_PROCESS:
            sys_create_process(trapframe);
            break;
        case JINUE_SYS_DUP:
            sys_dup(trapframe);
            break;
        case JINUE_SYS_CLOSE:
            sys_close(trapframe);
            break;
        case JINUE_SYS_DESTROY:
            sys_destroy(trapframe);
            break;
        case JINUE_SYS_MINT:
            sys_mint(trapframe);
            break;
        case JINUE_SYS_START_THREAD:
            sys_start_thread(trapframe);
            break;
        case JINUE_SYS_AWAIT_THREAD:
            sys_await_thread(trapframe);
            break;
        case JINUE_SYS_REPLY_ERROR:
            sys_reply_error(trapframe);
            break;
        case JINUE_SYS_SIGNAL_PROCESS:
            sys_signal_process(trapframe);
            break;
        case JINUE_SYS_SIGNAL_THREAD:
            sys_signal_thread(trapframe);
            break;
        case JINUE_SYS_RETURN_FROM_SIGNAL:
            sys_return_from_signal(trapframe);
            break;
        case JINUE_SYS_GET_SET_SIGNAL_MASK:
            sys_get_set_signal_mask(trapframe);
            break;
        default:
            sys_nosys(trapframe);
        }
    }
    else {
        /* inter-process message */
        sys_send(trapframe);
    }
}
