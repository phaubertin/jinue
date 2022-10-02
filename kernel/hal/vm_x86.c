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
#include <assert.h>
#include <boot.h>
#include <vmalloc.h>

/* This file contains non-Physical Address Extension (PAE) memory management
 * code. PAE code is located in vm_pae.c. Virtual memory management code
 * independent of PAE is located in vm.c. */

/** number of entries in a page directory or page table */
#define PAGE_TABLE_ENTRIES      VM_X86_PAGE_TABLE_PTES

struct pte_t {
    uint32_t entry;
};

void vm_x86_create_initial_addr_space(
        addr_space_t    *address_space,
        pte_t           *page_directory) {

    address_space->top_level.pd = (pte_t *)PHYS_TO_VIRT_AT_16MB(page_directory);
    address_space->cr3          = (uintptr_t)page_directory;
}

addr_space_t *vm_x86_create_addr_space(
        addr_space_t    *addr_space,
        pte_t           *page_directory) {

    addr_space->top_level.pd = page_directory;
    addr_space->cr3          = vm_lookup_kernel_paddr(page_directory);

    return addr_space;
}

void vm_x86_destroy_addr_space(addr_space_t *addr_space) {
    vm_destroy_page_directory(
            addr_space->top_level.pd,
            /* Free page tables for addresses 0..KLIMIT, be careful not to free
             * the kernel page tables starting at KLIMIT. */
            vm_x86_page_directory_offset_of((addr_t)KLIMIT));
}

/**
 * Get entry offset of specified virtual address within page table
 *
 * @param addr virtual address
 * @return entry offset of address within page table
 *
 */
unsigned int vm_x86_page_table_offset_of(void *addr) {
    return PAGE_TABLE_OFFSET_OF(addr);
}

/**
 * Get entry offset of specified virtual address within page directory
 *
 * @param addr virtual address
 * @return entry offset of address within page directory
 *
 */
unsigned int vm_x86_page_directory_offset_of(void *addr) {
    return PAGE_DIRECTORY_OFFSET_OF(addr);
}

/**
 * Lookup the page directory for a specified address and address space.
 *
 * This is the implementation for standard 32-bit (i.e. non-PAE) paging. This
 * means that there is only one preallocated page directory for the address
 * space.
 *
 * @param addr_space address space in which the address is looked up.
 */
pte_t *vm_x86_lookup_page_directory(addr_space_t *addr_space) {
    return addr_space->top_level.pd;
}

/**
 * Get page table entry (PTE) at specified entry offset from specified PTE
 *
 * @param pte base page table entry (e.g. the beginning of a page table)
 * @param offset entry offset
 * @return PTE at specified offset
 *
 */
pte_t *vm_x86_get_pte_with_offset(pte_t *pte, unsigned int offset) {
    return &pte[offset];
}

/**
 * Whether the specified page table or directory entry maps a page present in memory
 *
 * @param pte page table or page directory entry
 * @return true if page is present in memory, false otherwise
 *
 */
bool vm_x86_pte_is_present(const pte_t *pte) {
	/* Warning: this function will break for page directory entries if bit 11 is
	 * ever assigned. Currently, bit 11 is used for X86_PTE_PROT_NONE in page
	 * table entries and is unused and assumed to be zero in page directory
	 * entries. */
    return !!( pte->entry & (X86_PTE_PRESENT | X86_PTE_PROT_NONE));
}

/**
 * Filter out architecture-dependent flags
 *
 * This function filters out flags that are only relevant for PAE entries
 * (specifically, the No-eXecute/NX bit).
 *
 * The appropriate flags for this function are the architecture-dependent flags,
 * i.e. those defined by the X86_PTE_... constants. See map_page_access_flags()
 * for additional context.
 *
 * @param pte page table or page directory entry
 * @param paddr physical address of page frame
 * @param flags flags
 * @return flags
 *
 */
static uint32_t filter_pte__flags(uint64_t flags) {
    uint32_t mask = PAGE_MASK;
    return (flags & mask);
}

/**
 * Set page frame address and flags of the specified page table/directory entry
 *
 * The appropriate flags for this function are the architecture-dependent flags,
 * i.e. those defined by the X86_PTE_... constants. See map_page_access_flags()
 * for additional context.
 *
 * @param pte page table or page directory entry
 * @param paddr physical address of page frame
 * @param flags flags
 *
 */
void vm_x86_set_pte(pte_t *pte, uint32_t paddr, uint64_t flags) {
    assert((paddr & PAGE_MASK) == 0);
    pte->entry = paddr | filter_pte__flags(flags);
}

/**
 * Set protection and other flags on specified page table entry
 *
 * The appropriate flags for this function are the architecture-dependent flags,
 * i.e. those defined by the X86_PTE_... constants. See map_page_access_flags()
 * for additional context.
 *
 * @param pte page table entry
 * @param pte flags flags
 *
 */
void vm_x86_set_pte_flags(pte_t *pte, uint64_t flags) {
    pte->entry = (pte->entry & ~PAGE_MASK) | filter_pte__flags(flags);
}

/**
 * Get the physical address set in a page table or page directory entry
 *
 * @param pte page table or page directory entry array
 * @return physical address
 *
 */
uint32_t vm_x86_get_pte_paddr(const pte_t *pte) {
    return pte->entry & ~PAGE_MASK;
}

/**
 * Clear page table or page directory entry
 *
 * Once clear, the entry no longer refers to anything and is not considered
 * present in memory.
 *
 * @param pte page table or page directory entry
 *
 */
void vm_x86_clear_pte(pte_t *pte) {
    pte->entry = 0;
}

/**
 * Copy a page table or page directory entry
 *
 * @param dest destination page table/directory entry
 * @param src source page table/directory entry
 *
 */
void vm_x86_copy_pte(pte_t *dest, const pte_t *src) {
    dest->entry = src->entry;
}
