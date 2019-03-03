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

#ifndef _JINUE_ASM_VM_H
#define _JINUE_ASM_VM_H

#include <jinue-common/asm/types.h>


/** number of bits in virtual address for offset inside page */
#define PAGE_BITS               12

/** size of page */
#define PAGE_SIZE               (1<<PAGE_BITS) /* 4096 */

/** bit mask for offset in page */
#define PAGE_MASK               (PAGE_SIZE - 1)

/** The virtual address range starting at KLIMIT is reserved by the kernel. The
    region above KLIMIT has the same mapping in all address spaces. KLIMIT must
    be aligned on a page directory boundary in PAE mode. */
#define KLIMIT                  0xc0000000

/** limit of initial mapping performed by the 32-bit setup code */
#define KERNEL_EARLY_LIMIT      (KLIMIT + 2 * MB)

/** limit of the kernel image region */
#define KERNEL_IMAGE_END        (KLIMIT + 16 * MB)

/** limit up to which page tables are preallocated during kernel initialization */
#define KERNEL_PREALLOC_LIMIT   (KLIMIT + 32 * MB)

/** stack base address (stack top) */
#define STACK_BASE              KLIMIT

/** initial stack size */
#define STACK_SIZE              (8 * PAGE_SIZE)

/** initial stack lower address */
#define STACK_START             (STACK_BASE - STACK_SIZE)

#endif
