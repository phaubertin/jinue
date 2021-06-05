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

#ifndef JINUE_HAL_ASM_VM_H
#define JINUE_HAL_ASM_VM_H

#include <jinue-common/asm/vm.h>
#include <hal/asm/x86.h>

/** map page with no permission */
#define VM_MAP_NONE             0

/** map page with read permission */
#define VM_MAP_READ             VM_FLAG_READ_ONLY

/** map page with write permission */
#define VM_MAP_WRITE            VM_FLAG_READ_WRITE

/** map page with execution permission */
#define VM_MAP_EXEC             VM_FLAG_READ_ONLY

/** page is present in memory */
#define VM_FLAG_PRESENT         X86_PTE_PRESENT

/** page is read only */
#define VM_FLAG_READ_ONLY       0

/** page is read/write accessible */
#define VM_FLAG_READ_WRITE      X86_PTE_READ_WRITE

/** kernel mode page */
#define VM_FLAG_KERNEL          X86_PTE_GLOBAL

/** user mode page */
#define VM_FLAG_USER            X86_PTE_USER

/** Number of entries per page table/directory, PAE disabled */
#define VM_X86_PAGE_TABLE_PTES  1024

/** Number of entries per page table/directory, PAE enabled */
#define VM_PAE_PAGE_TABLE_PTES  512

#endif
