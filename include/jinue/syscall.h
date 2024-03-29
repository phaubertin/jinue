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

#ifndef _JINUE_SYSCALL_H
#define _JINUE_SYSCALL_H


#include <jinue/shared/asm/e820.h>
#include <jinue/shared/syscall.h>
#include <jinue/shared/types.h>
#include <stddef.h>

static inline void jinue_set_errno(int *perrno, int errval) {
    if(perrno != NULL) {
        *perrno = errval;
    }
}

uintptr_t jinue_syscall(jinue_syscall_args_t *args);

intptr_t jinue_syscall_with_usual_convention(jinue_syscall_args_t *args, int *perrno);

int jinue_set_syscall_implementation(int implementation, int *perrno);

void jinue_reboot(void);

void jinue_set_thread_local(void *addr, size_t size);

void *jinue_get_thread_local(void);

int jinue_create_thread(void (*entry)(), void *stack, int *perrno);

void jinue_yield_thread(void);

void jinue_exit_thread(void);

void jinue_putc(char c);

int jinue_puts(int loglevel, const char *str, size_t n, int *perrno);

int jinue_get_user_memory(jinue_mem_map_t *buffer, size_t buffer_size, int *perrno);

#endif
