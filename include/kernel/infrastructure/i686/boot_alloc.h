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

#ifndef JINUE_KERNEL_INFRASTRUCTURE_I686_BOOT_ALLOC_H
#define JINUE_KERNEL_INFRASTRUCTURE_I686_BOOT_ALLOC_H

#include <kernel/infrastructure/i686/types.h>
#include <kernel/interface/i686/types.h>

void boot_alloc_init(boot_alloc_t *boot_alloc, const bootinfo_t *bootinfo);

void boot_alloc_reinit_at_16mb(boot_alloc_t *boot_alloc);

void boot_alloc_reinit_at_klimit(boot_alloc_t *boot_alloc);

/**
 * Allocate an object on the boot heap.
 *
 * The object returned by the allocator is cleared (i.e. all bytes set to zero).
 *
 * This macro is a wrapper for boot_heap_alloc_size that takes a type as the
 * second argument instead of an object size.
 *
 * @param boot_alloc the boot allocator state
 * @param t the type of object to allocate
 * @param align the required start address alignment of the object, zero for no constraint
 * @return the allocated object
 *
 * */
#define boot_heap_alloc(boot_alloc, T, align) \
    ((T *)boot_heap_alloc_size(boot_alloc, sizeof(T), align))

void *boot_heap_alloc_size(boot_alloc_t *boot_alloc, size_t size, size_t align);

void *boot_page_alloc(boot_alloc_t *boot_alloc);

void *boot_page_alloc_n(boot_alloc_t *boot_alloc, int num_pages);

bool boot_page_alloc_is_empty(boot_alloc_t *boot_alloc);

#endif
