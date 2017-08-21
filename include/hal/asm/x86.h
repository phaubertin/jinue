/*
 * Copyright (C) 2017 Philippe Aubertin.
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

#ifndef JINUE_HAL_ASM_X86_H
#define JINUE_HAL_ASM_X86_H

#ifndef __ASSEMBLER__
#include <stdint.h>
#endif

/** CR0 register: Write Protect */
#define X86_CR0_WP                  (1<<16)

/** CR0 register: Paging */
#define X86_CR0_PG                  (1<<31)


/** CR4 register: Page Size Extension (PSE) */
#define X86_CR4_PSE                 (1<<4)

/** CR4 register: Physical Address Extension (PAE) */
#define X86_CR4_PAE                 (1<<5)

/** CR4 register: global pages */
#define X86_CR4_PGE                 (1<<7)


/** page is present in memory */
#define X86_PTE_PRESENT             (1<< 0)

/** page is read/write accessible */
#define X86_PTE_READ_WRITE          (1<< 1)

/** user mode page */
#define X86_PTE_USER                (1<< 2)

/** write-through cache policy for page */
#define X86_PTE_WRITE_THROUGH       (1<< 3)

/** uncached page */
#define X86_PTE_CACHE_DISABLE       (1<< 4)

/** page was accessed (read) */
#define X86_PTE_ACCESSED            (1<< 5)

/** page was written to */
#define X86_PTE_DIRTY               (1<< 6)

/** page directory entry describes a 4M page */
#define X86_PDE_PAGE_SIZE           (1<< 7)

/** page is global (mapped in every address space) */
#define X86_PTE_GLOBAL              (1<< 8)

/** do not execute bit */
#define X86_PTE_NX                  (UINT64_C(1)<< 63)

#endif
