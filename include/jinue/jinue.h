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

#ifndef _JINUE_JINUE_H
#define _JINUE_JINUE_H

#include <jinue/shared/asm/descriptors.h>
#include <jinue/shared/asm/errno.h>
#include <jinue/shared/asm/logging.h>
#include <jinue/shared/asm/machine.h>
#include <jinue/shared/asm/mman.h>
#include <jinue/shared/asm/memtype.h>
#include <jinue/shared/asm/permissions.h>
#include <jinue/shared/asm/stack.h>
#include <jinue/shared/asm/syscalls.h>
#include <jinue/shared/types.h>
#include <stddef.h>
#include <stdint.h>

int jinue_init(int implementation, int *perrno);

uintptr_t jinue_syscall(jinue_syscall_args_t *args);

void jinue_reboot(void);

void jinue_set_thread_local(void *addr, size_t size);

void *jinue_get_thread_local(void);

int jinue_create_thread(int fd, int process, int *perrno);

void jinue_yield_thread(void);

void jinue_exit_thread(void);

void jinue_putc(char c);

int jinue_puts(int loglevel, const char *str, size_t n, int *perrno);

int jinue_get_address_map(jinue_addr_map_t *buffer, size_t buffer_size, int *perrno);

int jinue_mmap(
        int          process,
        void        *addr,
        size_t       length,
        int          prot,
        uint64_t     paddr,
        int         *perrno);

intptr_t jinue_send(
        int                      fd,
        intptr_t                 function,
        const jinue_message_t   *message,
        int                     *perrno,
        uintptr_t               *perrcode);

intptr_t jinue_receive(int fd, const jinue_message_t *message, int *perrno);

intptr_t jinue_reply(const jinue_message_t *message, int *perrno);

int jinue_create_endpoint(int fd, int *perrno);

int jinue_create_process(int fd, int *perrno);

int jinue_mclone(
        int      src,
        int      dest,
        void    *src_addr,
        void    *dest_addr,
        size_t   length,
        int      prot,
        int     *perrno);

int jinue_dup(int process, int src, int dest, int *perrno);

int jinue_close(int fd, int *perrno);

int jinue_destroy(int fd, int *perrno);

int jinue_mint(
        int          owner,
        int          process,
        int          fd,
        int          perms,
        uintptr_t    cookie,
        int         *perrno);

int jinue_start_thread(int fd, void (*entry)(void), void *stack, int *perrno);

int jinue_await_thread(int fd, int *perrno);

int jinue_reply_error(uintptr_t errcode, int *perrno);

int jinue_set_acpi(jinue_acpi_tables_t *tables, int *perrno);

#endif
