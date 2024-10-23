/*
 * Copyright (C) 2019-2024 Philippe Aubertin.
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
#include <stdbool.h>
#include "machine.h"

void jinue_reboot(void) {
    jinue_syscall_args_t args;

    args.arg0 = JINUE_SYS_REBOOT;
    args.arg1 = 0;
    args.arg2 = 0;
    args.arg3 = 0;

    jinue_syscall(&args);
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

int jinue_create_thread(int fd, int process, int *perrno) {
    jinue_syscall_args_t args;

    args.arg0 = JINUE_SYS_CREATE_THREAD;
    args.arg1 = fd;
    args.arg2 = process;
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

void jinue_exit_thread(int status) {
    jinue_syscall_args_t args;

    args.arg0 = JINUE_SYS_EXIT_THREAD;
    args.arg1 = status;
    args.arg2 = 0;
    args.arg3 = 0;

    jinue_syscall(&args);
}

int jinue_puts(int loglevel, const char *str, size_t n, int *perrno) {
    jinue_syscall_args_t args;

    args.arg0 = JINUE_SYS_PUTS;
    args.arg1 = loglevel;
    args.arg2 = (uintptr_t)str;
    args.arg3 = n;

    return jinue_syscall_with_usual_convention(&args, perrno);
}

int jinue_get_user_memory(jinue_mem_map_t *buffer, size_t buffer_size, int *perrno) {
    jinue_syscall_args_t args;

    args.arg0 = JINUE_SYS_GET_USER_MEMORY;
    args.arg1 = (uintptr_t)buffer;
    args.arg2 = buffer_size;
    args.arg3 = 0;

    return jinue_syscall_with_usual_convention(&args, perrno);
}

int jinue_mmap(
        int          process,
        void        *addr,
        size_t       length,
        int          prot,
        uint64_t     paddr,
        int         *perrno) {

    jinue_syscall_args_t args;
    jinue_mmap_args_t mmap_args;

    mmap_args.addr = addr;
    mmap_args.length = length;
    mmap_args.prot = prot;
    mmap_args.paddr = paddr;

    args.arg0 = JINUE_SYS_MMAP;
    args.arg1 = process;
    args.arg2 = (uintptr_t)&mmap_args;
    args.arg3 = 0;

    return jinue_syscall_with_usual_convention(&args, perrno);
}

intptr_t jinue_send(
        int                      fd,
        intptr_t                 function,
        const jinue_message_t   *message,
        int                     *perrno) {

    jinue_syscall_args_t args;

    args.arg0 = (uintptr_t)function;
    args.arg1 = (uintptr_t)fd;
    args.arg2 = (uintptr_t)message;
    args.arg3 = 0;

    return jinue_syscall_with_usual_convention(&args, perrno);
}

intptr_t jinue_receive(int fd, const jinue_message_t *message, int *perrno){
    jinue_syscall_args_t args;

    args.arg0 = JINUE_SYS_RECEIVE;
    args.arg1 = (uintptr_t)fd;
    args.arg2 = (uintptr_t)message;
    args.arg3 = 0;

    return jinue_syscall_with_usual_convention(&args, perrno);
}

intptr_t jinue_reply(const jinue_message_t *message, int *perrno) {
    jinue_syscall_args_t args;

    args.arg0 = JINUE_SYS_REPLY;
    args.arg1 = 0;
    args.arg2 = (uintptr_t)message;
    args.arg3 = 0;

    return jinue_syscall_with_usual_convention(&args, perrno);
}

int jinue_create_endpoint(int fd, int *perrno) {
    jinue_syscall_args_t args;

    args.arg0 = JINUE_SYS_CREATE_ENDPOINT;
    args.arg1 = fd;
    args.arg2 = 0;
    args.arg3 = 0;

    return jinue_syscall_with_usual_convention(&args, perrno);
}

int jinue_create_process(int fd, int *perrno) {
    jinue_syscall_args_t args;

    args.arg0 = JINUE_SYS_CREATE_PROCESS;
    args.arg1 = fd;
    args.arg2 = 0;
    args.arg3 = 0;

    return jinue_syscall_with_usual_convention(&args, perrno);
}

int jinue_mclone(
        int      src,
        int      dest,
        void    *src_addr,
        void    *dest_addr,
        size_t   length,
        int      prot,
        int     *perrno) {

    jinue_syscall_args_t args;
    jinue_mclone_args_t mclone_args;

    mclone_args.src_addr = src_addr;
    mclone_args.dest_addr = dest_addr;
    mclone_args.length = length;
    mclone_args.prot = prot;

    args.arg0 = JINUE_SYS_MCLONE;
    args.arg1 = src;
    args.arg2 = dest;
    args.arg3 = (uintptr_t)&mclone_args;

    return jinue_syscall_with_usual_convention(&args, perrno);
}

int jinue_dup(int process, int src, int dest, int *perrno) {
    jinue_syscall_args_t args;

    args.arg0 = JINUE_SYS_DUP;
    args.arg1 = process;
    args.arg2 = src;
    args.arg3 = dest;

    return jinue_syscall_with_usual_convention(&args, perrno);
}

int jinue_close(int fd, int *perrno) {
    jinue_syscall_args_t args;

    args.arg0 = JINUE_SYS_CLOSE;
    args.arg1 = fd;
    args.arg2 = 0;
    args.arg3 = 0;

    return jinue_syscall_with_usual_convention(&args, perrno);
}

int jinue_destroy(int fd, int *perrno) {
    jinue_syscall_args_t args;

    args.arg0 = JINUE_SYS_DESTROY;
    args.arg1 = fd;
    args.arg2 = 0;
    args.arg3 = 0;

    return jinue_syscall_with_usual_convention(&args, perrno);
}

int jinue_mint(
        int          owner,
        int          process,
        int          fd,
        int          perms,
        uintptr_t    cookie,
        int         *perrno) {
    
    jinue_syscall_args_t args;
    jinue_mint_args_t mint_args;

    mint_args.process = process;
    mint_args.fd = fd;
    mint_args.perms = perms;
    mint_args.cookie = cookie;

    args.arg0 = JINUE_SYS_MINT;
    args.arg1 = owner;
    args.arg2 = (uintptr_t)&mint_args;
    args.arg3 = 0;

    return jinue_syscall_with_usual_convention(&args, perrno);
}

int jinue_start_thread(int fd, void (*entry)(void), void *stack, int *perrno) {
    jinue_syscall_args_t args;

    args.arg0 = JINUE_SYS_START_THREAD;
    args.arg1 = fd;
    args.arg2 = (uintptr_t)entry;
    args.arg3 = (uintptr_t)stack;

    return jinue_syscall_with_usual_convention(&args, perrno);
}
