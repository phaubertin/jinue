/*
 * Copyright (C) 2024-2026 Philippe Aubertin.
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

#ifndef JINUE_KERNEL_MACHINE_ASM_MACHINE_H
#define JINUE_KERNEL_MACHINE_ASM_MACHINE_H

#include <jinue/shared/asm/machine.h>

#ifdef JINUE_ARCH_IS_AMD64
#include <kernel/infrastructure/amd64/exports/asm/machine.h>
#endif

#ifdef JINUE_ARCH_IS_I686
#include <kernel/infrastructure/i686/exports/asm/machine.h>
#endif

/** number of bits in a virtual address that represent the offset within a page */
#define PAGE_BITS   JINUE_PAGE_BITS

/** size of a page */
#define PAGE_SIZE   JINUE_PAGE_SIZE

/** bit mask for offset in a page */
#define PAGE_MASK   JINUE_PAGE_MASK

/** size of region reserved for kernel image */
#define KERNEL_SIZE (1 * MB)

/** start of memory allocations */
#define ALLOC_BASE  (KERNEL_BASE + KERNEL_SIZE)

#endif
