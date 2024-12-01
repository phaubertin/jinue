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

#ifndef _JINUE_SHARED_TYPES_H
#define _JINUE_SHARED_TYPES_H

#include <stddef.h>
#include <stdint.h>

/** Arguments and return values for system calls
 *
 * When invoking a system call, arg0 contains the call number and arg1 to arg3
 * contain the arguments for the call. Call numbers SYSCALL_USER_BASE and up all
 * identify the send system call and the call number is passed to the message
 * recipient.
 *
 * On return from a system call, the contents of arg0 to arg3 depends on the
 * call. Most, but not all, system calls follow the following convention:
 *
 *  - arg0 contains a return value which should be cast as a signed integer
 *    (type int - see jinue_get_return()). If the value is positive (including
 *    zero), then the call was successful. A non-zero negative value indicates
 *    an error has occurred.
 *  - If the call failed, as indicated by the value in arg0, arg1 contains the
 *    error number.
 *  - arg2 and arg3 are reserved and should be ignored.
 *
 *  */
typedef struct {
    uintptr_t arg0;
    uintptr_t arg1;
    uintptr_t arg2;
    uintptr_t arg3;
} jinue_syscall_args_t;

typedef struct {
    void        *addr;
    size_t       size;
} jinue_buffer_t;

typedef struct {
    const void  *addr;
    size_t       size;
} jinue_const_buffer_t;

typedef struct {
    const jinue_const_buffer_t  *send_buffers;
    size_t                       send_buffers_length;
    const jinue_buffer_t        *recv_buffers;
    size_t                       recv_buffers_length;
    uintptr_t                    recv_function;
    uintptr_t                    recv_cookie;
    uintptr_t                    reply_max_size;
} jinue_message_t;

typedef struct {
    uint64_t    addr;
    uint64_t    size;
    uint32_t    type;
} jinue_addr_map_entry_t;

typedef struct {
    uint32_t                num_entries;
    jinue_addr_map_entry_t  entry[];
} jinue_addr_map_t;

typedef struct {
    void        *addr;
    size_t       length;
    int          prot;
    uint64_t     paddr;
} jinue_mmap_args_t;

typedef struct {
    void        *src_addr;
    void        *dest_addr;
    size_t       length;
    int          prot;
} jinue_mclone_args_t;

typedef struct {
    int         process;
    int         fd;
    int         perms;
    uintptr_t   cookie;
} jinue_mint_args_t;

typedef struct {
    const void *rsdt;
    const void *fadt;
    const void *madt;
} jinue_acpi_tables_t;

#endif
