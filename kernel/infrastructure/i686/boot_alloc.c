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

#include <kernel/domain/alloc/page_alloc.h>
#include <kernel/domain/alloc/vmalloc.h>
#include <kernel/domain/services/panic.h>
#include <kernel/infrastructure/i686/pmap/vm.h>
#include <kernel/infrastructure/i686/boot_alloc.h>
#include <kernel/utils/utils.h>
#include <stdbool.h>
#include <string.h>

/**
 * Initialize the boot allocator.
 *
 * The boot allocator is used for heap and page allocation early during kernel
 * initialization.
 *
 * This function sets up the allocator to allocate pages after the kernel loaded
 * at 0x100000 (1MB). Once the kernel has been moved to 0x1000000 (16MB), the
 * boot_reinit_at_16mb() function has to be called to start allocating pages
 * there.
 *
 * @param boot_alloc the allocator state initialized by this function
 * @param bootinfo boot information structure
 *
 * */
void boot_alloc_init(boot_alloc_t *boot_alloc, const bootinfo_t *bootinfo) {
    memset(boot_alloc, 0, sizeof(boot_alloc_t));
    boot_alloc->heap_ptr = bootinfo->boot_heap;
    /* TODO handle heap limit. */

    boot_alloc->current_page        = (void *)PTR_TO_PHYS_ADDR_AT_1MB(bootinfo->boot_end);
    boot_alloc->page_limit          = (char *)MEMORY_ADDR_1MB + BOOT_SIZE_AT_1MB;
    boot_alloc->first_page_at_16mb  = (char *)bootinfo->page_table_1mb + 15 * MB;
}

/**
 * Re-initialize the boot allocator after kernel image was moved.
 *
 * See boot_alloc_init() description for additional details.
 *
 * @param boot_alloc the allocator state initialized by this function
 *
 * */
void boot_reinit_at_16mb(boot_alloc_t *boot_alloc) {
    boot_alloc->current_page        = boot_alloc->first_page_at_16mb;
    boot_alloc->page_limit          = (char *)MEMORY_ADDR_16MB + BOOT_SIZE_AT_16MB;
}

/**
 * Re-initialize the boot allocator after switch to kernel address space.
 *
 * Once the address space has been switched, memory starting at physical address
 * 0x1000000 (i.e. 16MB) is no longer identity mapped at virtual address
 * 0x1000000 but is instead only mapped at KLIMIT.
 *
 * After this function is called, the boot page allocator continues allocating
 * sequentially from the same physical address somewhere after 0x1000000 but
 * returns the virtual address where the page is actually mapped.
 *
 * @param boot_alloc the allocator state initialized by this function
 *
 * */
void boot_reinit_at_klimit(boot_alloc_t *boot_alloc) {
    boot_alloc->current_page        = (void *)PHYS_TO_VIRT_AT_16MB(boot_alloc->current_page);
    boot_alloc->page_limit          = (char *)KLIMIT + BOOT_SIZE_AT_16MB;
}

/**
 * Allocate an object on the boot heap.
 *
 * The object returned by the allocator is cleared (i.e. all bytes set to zero).
 *
 * Callers do not call this function directly but instead use the boot_heap_alloc()
 * macro that takes a type as the second argument instead of an object size.
 *
 * @param boot_alloc the boot allocator state
 * @param size the size of the object to allocate, in bytes
 * @param align the required start address alignment of the object, zero for no constraint
 * @return the allocated object
 *
 * */
void *boot_heap_alloc_size(boot_alloc_t *boot_alloc, size_t size, size_t align) {
    if(align != 0) {
        boot_alloc->heap_ptr = ALIGN_END_PTR(boot_alloc->heap_ptr, align);
    }

    void *object            = boot_alloc->heap_ptr;
    boot_alloc->heap_ptr    = (char *)boot_alloc->heap_ptr + size;

    if(size > 0) {
        memset(object, 0, size);
    }

    return object;
}

/**
 * Push the current state of the boot allocator heap.
 *
 * All heap allocations performed after calling this function are freed by the
 * matching call to boot_heap_pop(). This function can be called multiple times
 * before calling boot_heap_pop(). Heap states pushed by this function are
 * popped by boot_heap_pop()() in the reverse order they were pushed.
 *
 * @param boot_alloc the boot allocator state
 *
 * */
void boot_heap_push(boot_alloc_t *boot_alloc) {
    struct boot_heap_pushed_state *pushed_state =
            boot_heap_alloc(boot_alloc, struct boot_heap_pushed_state, 0);

    pushed_state->next              = boot_alloc->heap_pushed_state;
    boot_alloc->heap_pushed_state   = pushed_state;
}

/**
 * Pop the last pushed boot allocator heap.
 *
 * This function frees all heap allocations performed since the matching call to
 * boot_heap_push().
 *
 * @param boot_alloc the boot allocator state
 *
 * */
void boot_heap_pop(boot_alloc_t *boot_alloc) {
    if(boot_alloc->heap_pushed_state == NULL) {
        panic("No more boot heap pushed state to pop.");
    }

    boot_alloc->heap_ptr            = boot_alloc->heap_pushed_state;
    boot_alloc->heap_pushed_state   = boot_alloc->heap_pushed_state->next;
}

/**
 * Early page allocation.
 *
 * This function allocates pages sequentially following the kernel image and the
 * setup code allocations. It is meant to be called early in the initialization
 * process, while the temporary page tables set up by the setup code are still
 * being used, which means before the kernel switches to the initial address
 * space it sets up.
 *
 * Returned pages are cleared (i.e. all bytes are set to zero). Calling this
 * function when no more pages are available leads to a kernel panic.
 *
 * This function must not be called once the kernel has switched away from the
 * page tables set up by the setup code to the initial address space it has set
 * up itself.
 *
 * @param boot_alloc the boot allocator state
 * @return address of allocated page
 *
 * */
void *boot_page_alloc(boot_alloc_t *boot_alloc) {
    return boot_page_alloc_n(boot_alloc, 1);
}

/**
 * Early allocation of multiple consecutive pages.
 *
 * See boot_page_alloc() description for details.
 *
 * @param boot_alloc the boot allocator state
 * @param num_pages number of pages to allocate
 * @return address of allocated page
 *
 * */
void *boot_page_alloc_n(boot_alloc_t *boot_alloc, int num_pages) {
    addr_t allocation_start = boot_alloc->current_page;

    boot_alloc->current_page = allocation_start + num_pages * PAGE_SIZE;

    if(boot_alloc->current_page > boot_alloc->page_limit) {
        panic("boot_page_alloc_n(): available memory exhausted");
    }

    /* These newly allocated pages may have data left from a previous boot which
     * may contain sensitive information. Let's clear them. */
    clear_pages(allocation_start, num_pages);

    return allocation_start;
}

/**
 * Whether or not the boot-time page allocator is empty
 *
 * The kernel panics if an attempt is made to allocate one or more pages with
 * boot_page_alloc() or boot_page_alloc_n() once the boot-time page allocator
 * is empty. This function allows that situation to be detected before
 * attempting the allocation.
 *
 * @param boot_alloc the boot allocator state
 * @return whether or not the boot-time page allocator is empty
 *
 * */
bool boot_page_alloc_is_empty(boot_alloc_t *boot_alloc) {
    return boot_alloc->current_page >= boot_alloc->page_limit;
}
