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

#include <hal/vm.h>
#include <page_alloc.h>
#include <string.h>
#include <vmalloc.h>


struct alloc_page {
    struct alloc_page *next;
};

static struct alloc_page *head_page = NULL;

/**
 * Allocate a page of kernel memory.
 *
 * Pages allocated by this function can be used for any purpose in the kernel,
 * e.g. as slabs for the slab allocator or as page tables.
 *
 * Pages allocated by this function are not guaranteed to be mapped in the
 * allocations region of the kernel address space (that is, the region managed
 * by vmalloc()). While most will be, pages originally allocated in the
 * image region during initialization by calling boot_page_alloc_image() can be
 * reclaimed with page_free() and then re-allocated by this function.
 *
 * @return allocated page
 *
 * */
void *page_alloc(void) {
    struct alloc_page *alloc_page = head_page;

    if(alloc_page != NULL) {
        head_page = alloc_page->next;
    }

    return alloc_page;
}

/**
 * Free a page of kernel memory.
 *
 * Pages freed by calling this function are available to be re-allocated by the
 * page_alloc() function. This function can be used to free pages allocated
 * by page_alloc() or to reclaim pages allocated during kernel initialization
 * by boot_page_alloc() or boot_page_alloc_image().
 *
 * @param page the page to free
 *
 * */
void page_free(void *page) {
    struct alloc_page *alloc_page = page;
    alloc_page->next    = head_page;
    head_page           = alloc_page;
}

/**
 * Check that pages are available to be allocated.
 *
 * Page availability can be checked with this function before calling either
 * page_alloc() or remove_page_frame().
 *
 * @return true if pages are available (one or more)
 *
 * */
bool page_alloc_is_empty(void) {
    return head_page == NULL;
}

/**
 * Map a page frame and add it to the page allocator.
 *
 * This function is used to implement a system call that allows userspace to
 * provide additional page frames to the kernel. This function fails when no
 * more pages of kernel address space can be allocated with vmalloc() to map
 * the provided page frame.
 *
 * @param paddr physical address of the provided page frame
 * @return true if the function succeeded
 *
 * */
bool add_page_frame(kern_paddr_t paddr) {
    void *page = vmalloc();

    if(page == NULL) {
        return false;
    }

    vm_map_kernel(page, paddr, VM_FLAG_READ_WRITE);

    /* Since this page is coming from userspace, is is important to clear it:
     * 1) The page may contain sensitive information, which we don't want to
     *    leak through Meltdown-like vulnerabilities; and
     * 2) Since the content is userspace-chosen, it could be used for kernel
     *    vulnerability exploits. */
    clear_page(page);
    page_free(page);

    return true;
}

/**
 * Remove a page frame from the allocator.
 *
 * This function is used implement a system call that allows userspace to
 * reclaim free kernel memory for its own use. The address space page is
 * freed with vmfree() and the physical address of the underlying page frame is
 * returned.
 *
 * @return physical address of the freed page frame, or PFNULL if none is available
 *
 * */
kern_paddr_t remove_page_frame(void) {
    void *page = page_alloc();

    if(page == NULL) {
        return PFNULL;
    }

    /* This page is going to userspace. Let's clear its content so we don't
     * leak information about the kernel's internal state that could be useful
     * for exploiting vulnerabilities. */
    clear_page(page);

    kern_paddr_t paddr = vm_lookup_kernel_paddr(page);

    vm_unmap_kernel(page);

    /* The page may be in the image region instead of the allocations region if
     * it was allocated during kernel initialization. */
    if(vmalloc_is_in_range(page)) {
        vmfree(page);
    }

    return paddr;
}

/**
 * Clear a page by writing all bytes to zero.
 *
 * @param page the page to clear
 *
 * */
void clear_page(void *page) {
    memset(page, 0, PAGE_SIZE);
}
