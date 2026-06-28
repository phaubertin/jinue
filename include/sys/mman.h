/*
 * Copyright (C) 2023-2026 Philippe Aubertin.
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

#ifndef _JINUE_LIBC_SYS_MMAN_H
#define _JINUE_LIBC_SYS_MMAN_H

#include <jinue/shared/asm/syscalls.h>
#include <sys/types.h>
#include <stddef.h>

#define PROT_NONE   JINUE_PROT_NONE

#define PROT_READ   JINUE_PROT_READ

#define PROT_WRITE  JINUE_PROT_WRITE

#define PROT_EXEC   JINUE_PROT_EXEC


#define MAP_UNCACHEABLE     JINUE_MAP_UNCACHEABLE

#define MAP_WRITE_COMBINE   JINUE_MAP_WRITE_COMBINE

/* Keep the flags below allocated downward starting from bit 31 since the
 * JINUE_MAP_xx flags are allocated starting from bit 0. */

#define MAP_SHARED          (1<<28)

#define MAP_PRIVATE         (1<<29)

#define MAP_FIXED           (1<<30)

#define MAP_ANONYMOUS       (1<<31)

#define MAP_ANON            MAP_ANONYMOUS


#define MAP_FAILED          ((void *)-1)


void *mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off);

#endif
