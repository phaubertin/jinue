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
#include <kernel/interface/i686/asm/boot.h>
#include <kernel/interface/i686/types.h>
#include <kernel/machine/asm/machine.h>
#include <kernel/utils/pmap.h>

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
    alloc_ptr = ALIGN_END_PTR(alloc_ptr, PAGE_SIZE);

    /* one page for the first 2MB of memory */
    bootinfo->page_table_1mb = (pte_t *)alloc_ptr;
    alloc_ptr += PAGE_SIZE;

    /* page tables for mapping at 0x1000000 (i.e. at 16MB) */
    bootinfo->page_table_16mb = (pte_t *)alloc_ptr;
    alloc_ptr += PAGE_SIZE * NUM_PAGES(BOOT_SIZE_AT_16MB) / NOPAE_PAGE_TABLE_PTES;

    /* one page table for mapping at JINUE_KLIMIT */
    bootinfo->page_table_klimit = (pte_t *)alloc_ptr;
    alloc_ptr += PAGE_SIZE;

    /* page directory */
    bootinfo->page_directory = (pte_t *)alloc_ptr;
    alloc_ptr += PAGE_SIZE;

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
 * Initialize page tables for the mapping at 1MB
 * 
 * Map the 1MB of memory that starts with the kernel image, followed by boot
 * stack/heap and initial memory allocations.
 * 
 * @param pte first page table entry
 * @param bootinfo boot information structure
 * @return next page table entry
 */
static pte_t *map_kernel_image_1mb(pte_t *pte, bootinfo_t *bootinfo) {
    size_t image_size   = (char *)bootinfo->image_top - (char *)bootinfo->image_start;
    size_t entries      = image_size >> PAGE_BITS;

    pte = map_linear(pte, entries, (uint32_t)bootinfo->image_start /* read only */);

    return map_linear(pte, 256 - entries, (uint32_t)bootinfo->image_top | X86_PTE_READ_WRITE);
}

/**
 * Initialize page tables for the mapping at JINUE_KLIMIT
 * 
 * Map the 1MB of memory that starts with the kernel image, followed by boot
 * stack/heap and initial memory allocations.
 * 
 * @param pte first page table entry
 * @param bootinfo boot information structure
 * @return next page table entry
 */
static pte_t *map_kernel(pte_t *pte, bootinfo_t *bootinfo) {
    pte_t *first    = pte;
    size_t entries  = ((char *)bootinfo->image_top - (char *)bootinfo->image_start) >> PAGE_BITS;

    /* Check if we were able to find and copy the data segment earlier. If we
     * weren't, let's just map the whole kernel image read/write and let the
     * kernel deal with it later.
     *
     * If we weren't able to find the data segment, its size has been set to
     * zero. */
    uint32_t flags = (bootinfo->data_size == 0) ? X86_PTE_READ_WRITE : 0;

    pte_t *after_image = map_linear(pte, entries, (uint32_t)bootinfo->image_start | flags);

    /* map kernel data segment (read/write) */
    if(bootinfo->data_size != 0) {
        char *image_at_klimit   = (char *)bootinfo->image_start + BOOT_OFFSET_FROM_1MB;
        size_t data_offset      = (char *)bootinfo->data_start - image_at_klimit;

        map_linear(
            &first[data_offset >> PAGE_BITS],
            bootinfo->data_size >> PAGE_BITS,
            bootinfo->data_physaddr | X86_PTE_READ_WRITE
        );
    }

    /* Map initial allocations, including kernel boot stack and heap, up to but
     * excluding initial page tables and page directory. */
    size_t length = (char *)bootinfo->page_table_1mb - (char *)bootinfo->image_top;
    pte_t *next = map_linear(
        after_image,
        length >> PAGE_BITS,
        (uint32_t)bootinfo->image_top | X86_PTE_READ_WRITE
    );

    /* clear remainder of page table */
    return clear_ptes(next, 1024 - (length >> PAGE_BITS));
}

void initialize_page_tables(bootinfo_t *bootinfo) {
    /* Map first 1MB read/write. This includes video memory. */
    pte_t *pte = map_linear(
        bootinfo->page_table_1mb,
        256,
        X86_PTE_READ_WRITE /* start address is 0 */
    );

    /* map kernel image and read/write memory that follows (1MB)*/
    pte = map_kernel_image_1mb(pte, bootinfo);

    /* clear remaining half of page table (2MB) */
    clear_ptes(pte, 512);

    /* Initialize pages table to map BOOT_SIZE_AT_16MB starting at 0x1000000
     * (16M). */
    map_linear(
        bootinfo->page_table_16mb,
        NUM_PAGES(BOOT_SIZE_AT_16MB),
        MEMORY_ADDR_16MB | X86_PTE_READ_WRITE
    );

    /* Map kernel image and read/write memory that follows (1MB). */
    map_kernel(bootinfo->page_table_klimit, bootinfo);
}

/**
 * Initialize the page directory
 * 
 * @param bootinfo boot information structure
 */
void initialize_page_directory(bootinfo_t *bootinfo) {
    /* clear the page directory */
    clear_ptes(bootinfo->page_directory, 1024);

    /* add entry for the first page table */
    map_linear(
        &bootinfo->page_directory[0],
        1,
        (uint32_t)(bootinfo->page_table_1mb) | X86_PTE_READ_WRITE
    );

    /* add entries for page tables for memory at 16MB */
    map_linear(
        &bootinfo->page_directory[MEMORY_ADDR_16MB >> 22],
        NUM_PAGES(BOOT_SIZE_AT_16MB) / NOPAE_PAGE_TABLE_PTES,
        (uint32_t)(bootinfo->page_table_16mb) | X86_PTE_READ_WRITE
    );

    /* add entry for the last page table */
    map_linear(
        &bootinfo->page_directory[JINUE_KLIMIT >> 22],
        1,
        (uint32_t)(bootinfo->page_table_klimit) | X86_PTE_READ_WRITE
    );
}