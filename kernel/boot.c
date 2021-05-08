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
#include <boot.h>
#include <page_alloc.h>
#include <panic.h>
#include <stdbool.h>
#include <string.h>
#include <vmalloc.h>
#include <util.h>

/**
 * Initialize the boot allocator.
 *
 * The boot allocator is used for heap and page allocation during kernel
 * initialization. After this function is called, the boot heap is ready to
 * use (see boot_heap_alloc()). However, the page and page frame allocators
 * require additional initialization by the machine-dependent code before they
 * can be used.
 *
 * @param boot_alloc the allocator state initialized by this function
 * @param boot_info boot information structure
 *
 * */
void boot_alloc_init(boot_alloc_t *boot_alloc, const boot_info_t *boot_info) {
    memset(boot_alloc, 0, sizeof(boot_alloc_t));
    boot_alloc->heap_ptr    = boot_info->boot_heap;
    boot_alloc->its_early   = true;
    /* TODO handle heap limit. */

    boot_alloc->kernel_vm_top      = boot_info->boot_end;
    boot_alloc->kernel_vm_limit    = (addr_t)KERNEL_EARLY_LIMIT;
    boot_alloc->kernel_paddr_top   = EARLY_VIRT_TO_PHYS(boot_alloc->kernel_vm_top);
    boot_alloc->kernel_paddr_limit = MEM_ADDR_1MB + 1 * MB;
}

/**
 * Allocate an object on the boot heap.
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
 * When the kernel is first entered, the setup code has set up temporary page
 * tables that map a contiguous region of physical memory (RAM) that contains
 * the kernel image at KLIMIT. The setup code itself allocates a few pages,
 * notably for the temporary page tables and for the boot stack and heap. These
 * pages are allocated sequentially just after the kernel image.
 *
 * This function allocates pages sequentially following the kernel image and the
 * setup code allocations. It is meant to be called early in the initialization
 * process, while the temporary page tables set up by the setup code are still
 * being used, which means before the kernel switches to the initial address
 * space it sets up.
 *
 * Because the page tables set up by the setup code are being used, there is a
 * fixed relation between the virtual address of the pages allocated by this
 * function and the physical address of the underlying page frames. This relation
 * is expressed by the EARLY_PTR_TO_PHYS_ADDR() macro.
 *
 * This function must not be called once the kernel has switched away from the
 * page tables set up by the setup code to the initial address space it has set
 * up itself. This function checks for this and triggers a kernel panic if it
 * happens.
 *
 * @param boot_alloc the boot allocator state
 * @return address of allocated page
 *
 * */
addr_t boot_page_alloc_early(boot_alloc_t *boot_alloc) {
    return boot_page_alloc_n_early(boot_alloc, 1);
}

/**
 * Early allocation of multiple consecutive pages.
 *
 * When the kernel is first entered, the setup code has set up temporary page
 * tables that map a contiguous region of physical memory (RAM) that contains
 * the kernel image at KLIMIT. The setup code itself allocates a few pages,
 * notably for the temporary page tables and for the boot stack and heap. These
 * pages are allocated sequentially just after the kernel image.
 *
 * This function allocates pages sequentially following the kernel image and the
 * setup code allocations. It is meant to be called early in the initialization
 * process, while the temporary page tables set up by the setup code are still
 * being used, which means before the kernel switches to the initial address
 * space it sets up.
 *
 * Because the page tables set up by the setup code are being used, there is a
 * fixed relation between the virtual address of the pages allocated by this
 * function and the physical address of the underlying page frames. This relation
 * is expressed by the EARLY_PTR_TO_PHYS_ADDR() macro.
 *
 * This function must not be called once the kernel has switched away from the
 * page tables set up by the setup code to the initial address space it has set
 * up itself. This function checks for this and triggers a kernel panic if it
 * happens.
 *
 * @param boot_alloc the boot allocator state
 * @param num_pages number of pages to allocate
 * @return address of allocated page
 *
 * */
addr_t boot_page_alloc_n_early(boot_alloc_t *boot_alloc, int num_pages) {
    /* Preconditions */
    if(! boot_alloc->its_early) {
        panic("boot_pgalloc_early() called too late");
    }

    if(boot_alloc->kernel_vm_top == NULL) {
        panic("boot_pgalloc_early(): allocator is uninitialized");
    }

    if(((uintptr_t)boot_alloc->kernel_vm_top & PAGE_MASK) != 0) {
        panic("boot_pgalloc_early(): bad kernel region top VM address alignment");
    }

    if(boot_alloc->kernel_paddr_top != EARLY_PTR_TO_PHYS_ADDR(boot_alloc->kernel_vm_top)) {
        panic("boot_pgalloc_early(): inconsistent allocator state");
    }

    /* address of allocated page */
    addr_t allocation_start = boot_alloc->kernel_vm_top;

    /* Update allocator state.
     *
     * In this early allocator function that is called while the temporary page
     * tables set up by the setup code are still being used, there is a fixed
     * relationship between virtual and physical addresses. */
    boot_alloc->kernel_vm_top      = allocation_start + num_pages * PAGE_SIZE;
    boot_alloc->kernel_paddr_top   = boot_alloc->kernel_paddr_top + num_pages * PAGE_SIZE;

    /* Check updated state against allocation limits. */
    if(boot_alloc->kernel_vm_top > boot_alloc->kernel_vm_limit) {
        panic("vmalloc_early(): kernel address space exhausted");
    }

    if(boot_alloc->kernel_paddr_top > boot_alloc->kernel_paddr_limit) {
        panic("vmalloc_early(): available memory exhausted");
    }

    /* These newly allocated pages may have data left from a previous boot which
     * may contain sensitive information. Let's clear them. */
    clear_pages(allocation_start, num_pages);

    /* Post-condition */
    if(boot_alloc->kernel_paddr_top != EARLY_PTR_TO_PHYS_ADDR(boot_alloc->kernel_vm_top)) {
        panic("boot_pgalloc_early(): inconsistent allocator state on return");
    }

    return allocation_start;
}

/**
 * Allocate a page frame, that is, a page of physical memory.
 *
 * The allocated page frame is not mapped anywhere. For a mapped page, call
 * boot_page_alloc() or boot_page_alloc_image() instead;
 *
 * @param boot_alloc the boot allocator state
 * @return physical address of allocated page frame
 *
 * */
kern_paddr_t boot_page_frame_alloc(boot_alloc_t *boot_alloc) {
    if(boot_alloc->its_early) {
        panic("boot_pfalloc() called too soon");
    }

    /* address of allocated page  */
    kern_paddr_t paddr = boot_alloc->kernel_paddr_top;

    /* Update allocator state. */
    boot_alloc->kernel_paddr_top = paddr + PAGE_SIZE;

    /* Check bounds. */
    if(boot_alloc->kernel_paddr_top > boot_alloc->kernel_paddr_limit) {
        panic("pfalloc_boot(): available memory exhausted");
    }

    return paddr;
}

/**
 * Allocate a page of address space.
 *
 * No memory is mapped to the allocated page. The page is allocated from the
 * image region of the kernel address space, just after the kernel image and
 * other initialization-time page allocations. This function allocates pages
 * sequentially.
 *
 * @param boot_alloc the boot allocator state
 * @return address of allocated page
 *
 * */
addr_t boot_vmalloc(boot_alloc_t *boot_alloc) {
    if(boot_alloc->its_early) {
        panic("boot_vmalloc() called too soon");
    }

    if(boot_alloc->kernel_vm_top == NULL) {
        panic("boot_pgalloc_early(): allocator is uninitialized");
    }

    /* address of allocated page */
    addr_t page = boot_alloc->kernel_vm_top;

    /* Update allocator state. */
    boot_alloc->kernel_vm_top = page + PAGE_SIZE;

    /* Check bounds. */
    if(boot_alloc->kernel_vm_top > boot_alloc->kernel_vm_limit) {
        panic("vmalloc_boot(): kernel address space exhausted");
    }

    return page;
}

/**
 * Allocate a page in the allocations region of the kernel address space.
 *
 * The physical memory is allocated just after the kernel image and other
 * initialization-time allocations by calling boot_page_frame_alloc() whereas
 * the address space page is allocated in the allocations region by calling
 * vmalloc().
 *
 * If either of these two conditions is met, you must use boot_pgalloc_image()
 * instead of this function:
 * 1) The address space page allocator has not not yet been initialized by
 *    calling vmalloc_init(); or
 * 2) It is necessary to allocate multiple contiguous pages.
 *
 * @param boot_alloc the boot allocator state
 * @return address of allocated page
 *
 * */
addr_t boot_page_alloc(boot_alloc_t *boot_alloc) {
    kern_paddr_t paddr  = boot_page_frame_alloc(boot_alloc);
    addr_t vaddr        = vmalloc();

    vm_map_kernel(vaddr, paddr, VM_FLAG_READ_WRITE);

    /* This newly allocated page may have data left from a previous boot which
     * may contain sensitive information. Let's clear it. */
    clear_page(vaddr);

    return vaddr;
}

/**
 * Allocate a page in the image region of the kernel address space.
 *
 * Since the size of the image region is limited, use boot_page_alloc() instead
 * of this function whenever possible.
 *
 * The difference between this function and boot_page_alloc() is that the
 * address space page is allocated by this function using boot_vmalloc() instead
 * of vmalloc(). Pages allocated by consecutive calls to this function are
 * allocated sequentially.
 *
 * @param boot_alloc the boot allocator state
 * @return address of allocated page
 *
 * */
addr_t boot_page_alloc_image(boot_alloc_t *boot_alloc) {
    kern_paddr_t paddr  = boot_page_frame_alloc(boot_alloc);
    addr_t vaddr        = boot_vmalloc(boot_alloc);

    vm_map_kernel(vaddr, paddr, VM_FLAG_READ_WRITE);

    /* This newly allocated page may have data left from a previous boot which
     * may contain sensitive information. Let's clear it. */
    clear_page(vaddr);

    return vaddr;
}
