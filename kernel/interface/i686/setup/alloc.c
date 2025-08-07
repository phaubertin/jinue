/*
 * Copyright (C) 2025 Philippe Aubertin.
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

#include <kernel/interface/i686/setup/alloc.h>
#include <kernel/machine/asm/machine.h>
#include <kernel/utils/utils.h>

void *alloc_heap(bootinfo_t *bootinfo, size_t size, size_t align) {
    if(align != 0) {
        bootinfo->boot_heap = ALIGN_END_PTR(bootinfo->boot_heap, align);
    }

    void *object        = bootinfo->boot_heap;
    bootinfo->boot_heap = (char *)bootinfo->boot_heap + size;

    return object;
}

void *alloc_pages(bootinfo_t *bootinfo, size_t size) {
    void *retval = bootinfo->boot_end;
    
    bootinfo->boot_end  = ALIGN_END_PTR((char *)retval + size, PAGE_SIZE);

    return retval;
}

void set_alloc_pages_address(bootinfo_t *bootinfo, void *addr) {
    bootinfo->boot_end = addr;
}
