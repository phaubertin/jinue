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

#include <hal/vm_private.h>
#include <boot.h>
#include <pfalloc.h>
#include <vmalloc.h>

/** number of entries in a page directory or page table */
#define PAGE_TABLE_ENTRIES      VM_PAE_PAGE_TABLE_PTES

struct pte_t {
    uint32_t entry;
};

void vm_x86_boot_init(void) {
    page_table_entries = (size_t)PAGE_TABLE_ENTRIES;
}

addr_space_t *vm_x86_create_addr_space(addr_space_t *addr_space) {
    /* Create a new page directory where entries for the address range starting
     * at KLIMIT are copied from the initial address space. The mappings starting
     * at KLIMIT belong to the kernel and are identical in all address spaces. */
    kern_paddr_t paddr = vm_clone_page_directory(
            initial_addr_space.top_level.pd,
            vm_x86_page_directory_offset_of((addr_t)KLIMIT));

    addr_space->top_level.pd = paddr;
    addr_space->cr3          = paddr;

    return addr_space;
}

addr_space_t *vm_x86_create_initial_addr_space(boot_alloc_t *boot_alloc) {
    pte_t *page_directory = (pte_t *)boot_page_alloc_early(boot_alloc);

    vm_init_initial_page_directory(
            page_directory,
            boot_alloc,
            vm_x86_page_directory_offset_of((addr_t)KLIMIT),
            vm_x86_page_directory_offset_of((addr_t)KERNEL_PREALLOC_LIMIT),
            true);

    initial_addr_space.top_level.pd = EARLY_PTR_TO_PHYS_ADDR(page_directory);
    initial_addr_space.cr3          = EARLY_VIRT_TO_PHYS((uintptr_t)page_directory);

    return &initial_addr_space;
}

void vm_x86_destroy_addr_space(addr_space_t *addr_space) {
    vm_destroy_page_directory(
            addr_space->top_level.pd,
            /* Free page tables for addresses 0..KLIMIT, be careful not to free
             * the kernel page tables starting at KLIMIT. */
            0,
            vm_x86_page_directory_offset_of((addr_t)KLIMIT));
}

unsigned int vm_x86_page_table_offset_of(addr_t addr) {
    return PAGE_TABLE_OFFSET_OF(addr);
}

unsigned int vm_x86_page_directory_offset_of(addr_t addr) {
    return PAGE_DIRECTORY_OFFSET_OF(addr);
}

/**
    Lookup and map the page directory for a specified address and address space.

    This is the implementation for standard 32-bit (i.e. non-PAE) paging. This
    means that there is only one preallocated page directory, so the addr and
    create_as_needed arguments are both irrelevant.

    Important note: it is the caller's responsibility to unmap and free the returned
    page directory when it is done with it.

    @param addr_space address space in which the address is looked up.
    @param addr address to look up
    @param create_as_need Whether a page table is allocated if it does not exist
*/
pte_t *vm_x86_lookup_page_directory(addr_space_t *addr_space) {
    pte_t *page_directory = (pte_t *)vmalloc();
    vm_map_kernel((addr_t)page_directory, addr_space->top_level.pd, VM_FLAG_READ_WRITE);

    return page_directory;
}

pte_t *vm_x86_get_pte_with_offset(pte_t *pte, unsigned int offset) {
    return &pte[offset];
}

void vm_x86_set_pte(pte_t *pte, uint32_t paddr, int flags) {
    pte->entry = paddr | flags;
}

void vm_x86_set_pte_flags(pte_t *pte, int flags) {
    pte->entry = (pte->entry & ~PAGE_MASK) | flags;
}

int vm_x86_get_pte_flags(const pte_t *pte) {
    return pte->entry & PAGE_MASK;
}

uint32_t vm_x86_get_pte_paddr(const pte_t *pte) {
    return pte->entry & ~PAGE_MASK;
}

void vm_x86_clear_pte(pte_t *pte) {
    pte->entry = 0;
}

void vm_x86_copy_pte(pte_t *dest, const pte_t *src) {
    dest->entry = src->entry;
}
