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

#ifndef _JINUE_COMMON_VM_H
#define _JINUE_COMMON_VM_H

#include <jinue/shared/asm/vm.h>

#include <stdbool.h>
#include <stdint.h>


/** byte offset in page of virtual (linear) address */
#define page_offset_of(x)   ((uintptr_t)(x) & PAGE_MASK)

/** address of the page that contains a virtual (linear) address */
#define page_address_of(x)  ((uintptr_t)(x) & ~PAGE_MASK)

/** sequential page number of virtual (linear) address */
#define page_number_of(x)   ((uintptr_t)(x) >> PAGE_BITS)

/** Check whether a pointer points to kernel space */
static inline bool is_kernel_pointer(const void *addr) {
    return (uintptr_t)addr >= KLIMIT;
}

/** Check whether a pointer points to user space */
static inline bool is_userspace_pointer(const void *addr) {
    return (uintptr_t)addr < KLIMIT;
}

/** Maximum size of user buffer starting at specified address */
static inline uintptr_t user_pointer_max_size(const void *addr) {
    return (uintptr_t)0 - (uintptr_t)addr;
}

/** Check that a buffer is completely in user space */
static inline bool check_userspace_buffer(const void *addr, uintptr_t size) {
    return is_userspace_pointer(addr) && size <= user_pointer_max_size(addr);
}

#endif
