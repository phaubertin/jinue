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

#ifndef _JINUE_SHARED_ASM_I686_H
#define _JINUE_SHARED_ASM_I686_H

/** This file contains machine-specific definitions that need to be visible
 * outside the machine-specific parts of the code, both in user space and
 * the kernel.
 * 
 * It is OK to include this file from i686-specific code in the kernel.
 * Otherwise, it should be included through either <jinue/jinue.h> or
 * or <kernel/types.h> (which includes <kernel/machine/types.h>). */


/** number of bits in a virtual address that represent the offset within a page */
#define JINUE_PAGE_BITS                     12

/** size of a page */
#define JINUE_PAGE_SIZE                     (1<<JINUE_PAGE_BITS)  /* 4096 */

/** bit mask for offset in a page */
#define JINUE_PAGE_MASK                     (JINUE_PAGE_SIZE - 1)

/** The virtual address range starting at JINUE_KLIMIT is reserved by the
 * kernel. The region above JINUE_KLIMIT has the same mapping in all address
 * spaces. JINUE_KLIMIT must be aligned on a page directory boundary in PAE
 * mode. */
#define JINUE_KLIMIT                        0xc0000000

/** interrupt vector for system call software interrupt */
#define JINUE_I686_SYSCALL_INTERRUPT        0x80

/** slow/safe interrupt-based system call implementation */
#define JINUE_I686_HOWSYSCALL_INTERRUPT     0

/** AMD's fast system call implementation (SYSCALL/SYSLEAVE) */
#define JINUE_I686_HOWSYSCALL_FAST_AMD      1

/** Intel's fast system call implementation (SYSENTER/SYSEXIT) */
#define JINUE_I686_HOWSYSCALL_FAST_INTEL    2

/** last system call implementation index */
#define JINUE_I686_HOWSYSCALL_LAST          2

#endif
