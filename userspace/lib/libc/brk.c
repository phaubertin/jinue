/*
 * Copyright (C) 2023 Philippe Aubertin.
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

#include <jinue/jinue.h>
#include <sys/auxv.h>
#include <sys/elf.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include "mmap.h"
#include "physmem.h"

/* Original value of the program break, i.e. just after the data segment. We
 * cannot go lower than this. */
static void *bottom_break;

/* program break reported by sbrk(0), etc. */
static void *reported_break;

/* We only allocate, never free, we just pretend we do. This is how far we
 * allocated. Also, this is the actual allocated limit, aligned on a page
 * boundary. */
static void *allocated_break;

int brk_init(void) {
    Elf32_Phdr *phdr = (Elf32_Phdr *)getauxval(JINUE_AT_PHDR);

    if(phdr == NULL) {
        return EXIT_FAILURE;
    }

    size_t entry_size = getauxval(JINUE_AT_PHENT);

    if(entry_size == 0) {
        return EXIT_FAILURE;
    }

    int num_entries = getauxval(JINUE_AT_PHNUM);

    if(num_entries < 1) {
        return EXIT_FAILURE;
    }

    int idx;

    for(idx = 0; idx < num_entries; ++idx) {
        if(phdr->p_type == PT_LOAD && (phdr->p_flags & PF_W)) {
            break;
        }

        phdr = (void *)((char *)phdr + entry_size);
    }

    if(idx == num_entries) {
        return EXIT_FAILURE;
    }

    reported_break  = (void *)(phdr->p_vaddr + phdr->p_memsz);
    bottom_break    = reported_break;
    allocated_break = (void *)(((uintptr_t)reported_break + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1));

    return EXIT_SUCCESS;
}

int brk(void *addr) {
    if((uintptr_t)addr < (uintptr_t)bottom_break) {
        errno = EINVAL;
        return -1;
    }

    if((uintptr_t)addr > MMAP_BASE) {
        errno = ENOMEM;
        return -1;
    }

    if((uintptr_t)addr > (uintptr_t)allocated_break) {
        uintptr_t new_allocated = ((uintptr_t)addr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
        size_t size             = new_allocated - (uintptr_t)allocated_break;
        intptr_t physaddr       = physmem_alloc(size);

        if(physaddr < 0) {
            errno = ENOMEM;
            return -1;
        }

        int kernel_errno;
        int ret = jinue_mmap(
                JINUE_SELF_PROCESS_DESCRIPTOR,
                allocated_break,
                size,
                JINUE_PROT_READ | JINUE_PROT_WRITE,
                physaddr,
                &kernel_errno);

        if(ret < 0) {
            errno = kernel_errno;
            return -1;
        }

        allocated_break = (void *)new_allocated;
    }

    reported_break = addr;

    return 0;
}

void *sbrk(intptr_t incr) {
    void *previous_break = reported_break;

    int ret = brk((char *)reported_break + incr);

    if(ret != 0) {
        return (void *)-1;
    }

    return previous_break;
}
