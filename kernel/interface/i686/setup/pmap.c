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

#include <kernel/infrastructure/i686/pmap/asm/pmap.h>
#include <kernel/infrastructure/i686/pmap/pmap.h>
#include <kernel/interface/i686/asm/boot.h>
#include <kernel/interface/i686/setup/pmap.h>
#include <kernel/interface/i686/types.h>
#include <kernel/machine/asm/machine.h>
#include <kernel/utils/pmap.h>
#include <stddef.h>
#include <stdint.h>

struct pte_t {
    uint32_t entry;
};

/**
 * Allocate initial non-PAE page tables and page directory
 * 
 * @param alloc_ptr allocation pointer, i.e. where to allocate memory
 * @param bootinfo boot information structure
 * @return updated allocation pointer
 */
char *allocate_page_tables(char *alloc_ptr, bootinfo_t *bootinfo) {
    /* align to page boundary */
    alloc_ptr = ALIGN_END_PTR(alloc_ptr, PAGE_SIZE);

    /* TODO deprecated, remove these */
    bootinfo->page_table_1mb = NULL;
    bootinfo->page_table_16mb = NULL;

    /* kernel page tables */
    int num_pages       = NUM_PAGES(ADDR_4GB - JINUE_KLIMIT);
    int num_page_tables = num_pages / 1024;

    bootinfo->page_tables = (pte_t *)alloc_ptr;
    alloc_ptr += num_page_tables * PAGE_SIZE;

    /* page directory */
    bootinfo->page_directory = (pte_t *)alloc_ptr;
    alloc_ptr += PAGE_SIZE;

    bootinfo->cr3       = (uint32_t)bootinfo->page_directory;
    bootinfo->use_pae   = false;

    return alloc_ptr;
}

/**
 * Clear consecutive page table entries
 * 
 * @param pte first page table entry
 * @param n number of entries to clear
 * @return next page table entry
 */
static pte_t *clear_ptes(pte_t *pte, size_t n) {
    for(size_t idx = 0; idx < n; ++idx) {
        pte->entry = 0;
        ++pte;
    }

    return pte;
}

/**
 * Initialize consecutive page table entries to map consecutive memory
 * 
 * @param pte first page table entry
 * @param n number of entries to clear
 * @param value first physical address and flags
 * @return next page table entry
 */
static pte_t *map_linear(pte_t *pte, size_t n, uint32_t value) {
    value |= X86_PTE_PRESENT;

    for(size_t idx = 0; idx < n; ++idx) {
        pte->entry = value;
        value += PAGE_SIZE;
        ++pte;
    }

    return pte;
}

/**
 * Initialize the page tables
 * 
 * @param bootinfo boot information structure
 */
void initialize_page_tables(bootinfo_t *bootinfo) {
    /* map the kernel image
     *
     * Check if we were able to find and copy the data segment earlier. If we
     * weren't, let's just map the whole kernel image read/write and let the
     * kernel deal with it later.
     *
     * If we weren't able to find the data segment, its size has been set to
     * zero.
     * 
     * TODO should we really be doing this? */
    pte_t *page_tables = bootinfo->page_tables;

    clear_ptes(page_tables, NUM_PAGES(ADDR_4GB - JINUE_KLIMIT));

    uint32_t rwflag = (bootinfo->data_size == 0) ? X86_PTE_READ_WRITE : 0;
    size_t entries  = ((char *)bootinfo->image_top - (char *)bootinfo->image_start) >> PAGE_BITS;

    map_linear(
        &page_tables[(KERNEL_BASE - JINUE_KLIMIT) >> PAGE_BITS],
        entries,
        (uint32_t)bootinfo->image_start | rwflag | X86_PTE_GLOBAL
    );

    /* map kernel data segment (read/write) */
    if(bootinfo->data_size != 0) {
        size_t data_offset = (uintptr_t)bootinfo->data_start - JINUE_KLIMIT;

        map_linear(
            &page_tables[data_offset >> PAGE_BITS],
            NUM_PAGES(bootinfo->data_size),
            bootinfo->data_physaddr | X86_PTE_READ_WRITE | X86_PTE_GLOBAL
        );
    }

    /* map memory allocations */
    map_linear(
        &page_tables[(ALLOC_BASE - JINUE_KLIMIT) >> PAGE_BITS],
        NUM_PAGES(BOOT_SIZE_AT_16MB),
        MEMORY_ADDR_16MB | X86_PTE_READ_WRITE | X86_PTE_GLOBAL
    );

    /* link page tables in page directory */
    pte_t *page_directory = bootinfo->page_directory;

    clear_ptes(page_directory, 1024);

    map_linear(
        &page_directory[JINUE_KLIMIT >> (PAGE_BITS + 10)],
        NUM_PAGES(ADDR_4GB - JINUE_KLIMIT) / 1024,
        (uint32_t)page_tables | X86_PTE_READ_WRITE
    );
}

/**
 * Create temporary mappings for enabling paging
 * 
 * This function creates 1:1 mappings for the kernel image and initial memory
 * allocations so execution can continue once paging is enabled while some
 * pointers, including the stack and instruction pointers, still have their
 * paging disabled/physical address value. The pointers get adjusted and then
 * cleanup_after_paging() removes theses temporary mappings.
 * 
 * This function allocates a few page tables for the temporary mappings but
 * these page tables are discarded once the temporary mappings are no longer
 * needed.
 * 
 * @param alloc_ptr allocation pointer, i.e. where to allocate memory
 * @param bootinfo boot information structure
 */
void prepare_for_paging(char *alloc_ptr, const bootinfo_t *bootinfo) {
    /* mappings for the kernel image at 0x100000 (1MB) */
    pte_t *page_tables_1mb = (pte_t *)alloc_ptr;
    alloc_ptr += PAGE_SIZE;

    clear_ptes(page_tables_1mb, 1024);

    uint32_t flags  = (bootinfo->data_size == 0) ? X86_PTE_READ_WRITE : 0;
    size_t entries  = ((char *)bootinfo->image_top - (char *)bootinfo->image_start) >> PAGE_BITS;

    map_linear(
        &page_tables_1mb[(KERNEL_BASE - BOOT_OFFSET_FROM_1MB) >> PAGE_BITS],
        entries,
        (uint32_t)bootinfo->image_start | flags
    );
    
    /* mappings for the initial memory allocations at 0x1000000 (16MB) */
    pte_t *page_tables_16mb = (pte_t *)alloc_ptr;

    clear_ptes(page_tables_16mb, NUM_PAGES(BOOT_SIZE_AT_16MB));

    map_linear(
        page_tables_16mb,
        NUM_PAGES(BOOT_SIZE_AT_16MB),
        MEMORY_ADDR_16MB | X86_PTE_READ_WRITE
    );

    /* Link the temporary page tables into the page directory. */
    pte_t *page_directory = bootinfo->page_directory;

    map_linear(
        &page_directory[(KERNEL_BASE - BOOT_OFFSET_FROM_1MB) >> (PAGE_BITS + 10)],
        1,
        (uint32_t)page_tables_1mb | X86_PTE_READ_WRITE
    );

    map_linear(
        &page_directory[MEMORY_ADDR_16MB >> (PAGE_BITS + 10)],
        NUM_PAGES(BOOT_SIZE_AT_16MB) / 1024,
        (uint32_t)page_tables_16mb | X86_PTE_READ_WRITE
    );
}

/**
 * Remove the mapping created by prepare_for_paging()
 * 
 * @param bootinfo boot information structure
 */
void cleanup_after_paging(const bootinfo_t *bootinfo) {
    pte_t *page_directory = bootinfo->page_directory;

    clear_ptes(&page_directory[(KERNEL_BASE - BOOT_OFFSET_FROM_1MB) >> (PAGE_BITS + 10)], 1);

    clear_ptes(
        &page_directory[MEMORY_ADDR_16MB >> (PAGE_BITS + 10)],
        NUM_PAGES(BOOT_SIZE_AT_16MB) / 1024
    );
}
