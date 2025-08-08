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
#include <kernel/interface/i686/setup/alloc.h>
#include <kernel/interface/i686/setup/elf.h>
#include <kernel/interface/i686/setup/pmap.h>
#include <kernel/interface/i686/setup/setup32.h>
#include <kernel/interface/i686/types.h>
#include <kernel/machine/asm/machine.h>
#include <kernel/utils/pmap.h>
#include <sys/elf.h>
#include <stddef.h>
#include <stdint.h>

struct pte_t {
    uint32_t entry;
};

/**
 * Allocate initial non-PAE page tables and page directory
 * 
 * @param bootinfo boot information structure
 */
void allocate_page_tables(bootinfo_t *bootinfo) {
    /* Detect PAE support */
    bootinfo->use_pae       = detect_pae();
    size_t per_table_bits   = bootinfo->use_pae ? 9 : 10;

    /* kernel page tables */
    bootinfo->page_tables = alloc_pages(
        bootinfo,
        NUM_PAGES(ADDR_4GB - JINUE_KLIMIT) / (1 << per_table_bits) * PAGE_SIZE
    );
    
    /* page directory */
    bootinfo->page_directory = alloc_pages(bootinfo, PAGE_SIZE);

    if(!bootinfo->use_pae) {
        bootinfo->cr3 = (uint32_t)bootinfo->page_directory;
    }
    else {
        /* align */
        bootinfo->boot_heap = ALIGN_END_PTR(bootinfo->boot_heap, 32);

        /* allocate PDPT */
        bootinfo->cr3       = (uint32_t)alloc_heap(bootinfo, 32, 32);
    }
}

/**
 * Clear consecutive page table entries
 * 
 * @param use_pae whether PAE is enabled
 * @param table first page table entry of table
 * @param offset index of first entry to clear from start of table
 * @param n number of entries to clear
 * @return next page table entry
 */
static pte_t *clear_ptes(bool use_pae, pte_t *table, size_t offset, size_t n) {
    uint64_t *pte64 = (uint64_t *)table + offset;
    uint32_t *pte32 = (uint32_t *)table + offset;

    for(size_t idx = 0; idx < n; ++idx) {
        if(use_pae) {
            pte64[idx] = 0;
        }
        else {
            pte32[idx] = 0;
        }
    }

    return table;
}

/**
 * Initialize consecutive page table entries to map consecutive memory
 * 
 * @param use_pae whether PAE is enabled
 * @param table start of table
 * @param offset index of first entry to map from start of table
 * @param n number of entries to map
 * @param value first physical address and flags
 * @return next page table entry
 */
static pte_t *map_linear(bool use_pae, pte_t *table, size_t offset, size_t n, uint64_t value) {
    uint64_t *pte64 = (uint64_t *)table + offset;
    uint32_t *pte32 = (uint32_t *)table + offset;

    value |= X86_PTE_PRESENT;

    for(size_t idx = 0; idx < n; ++idx) {
        if(use_pae) {
            pte64[idx] = value;
        }
        else {
            pte32[idx] = value;
        }

        value += PAGE_SIZE;
    }

    return table;
}

/**
 * Initialize the page tables
 * 
 * @param bootinfo boot information structure
 */
void initialize_page_tables(bootinfo_t *bootinfo) {
    /* map the kernel image */
    clear_ptes(
        bootinfo->use_pae,
        bootinfo->page_tables,
        0,
        NUM_PAGES(ADDR_4GB - JINUE_KLIMIT)
    );

    map_linear(
        bootinfo->use_pae,
        bootinfo->page_tables,
        (KERNEL_BASE - JINUE_KLIMIT) >> PAGE_BITS,
        ((char *)bootinfo->image_top - (char *)bootinfo->image_start) >> PAGE_BITS,
        (uint32_t)bootinfo->image_start | X86_PTE_GLOBAL | X86_PTE_NX
    );

    /* make sure this setup code is executable */
    map_linear(
        bootinfo->use_pae,
        bootinfo->page_tables,
        (KERNEL_BASE - JINUE_KLIMIT) >> PAGE_BITS,
        1,
        (uint32_t)bootinfo->image_start | X86_PTE_GLOBAL
    );

    /* make sure kernel code segment is executable */
    const Elf32_Phdr *phdr = kernel_code_program_header(bootinfo);

    if(phdr != NULL) {
        uintptr_t code_vaddr    = ALIGN_START((uintptr_t)phdr->p_vaddr, PAGE_SIZE);
        size_t code_size        = phdr->p_memsz + OFFSET_OF_PTR(phdr->p_vaddr, PAGE_SIZE);
        size_t code_offset      = page_number_of(code_vaddr - JINUE_KLIMIT);

        map_linear(
            bootinfo->use_pae,
            bootinfo->page_tables,
            code_offset,
            NUM_PAGES(code_size),
            VIRT_TO_PHYS_AT_1MB(code_vaddr) | X86_PTE_GLOBAL
        );
    }

    /* map kernel data segment (read/write) */
    if(bootinfo->data_size != 0) {
        size_t data_offset = (uintptr_t)bootinfo->data_start - JINUE_KLIMIT;

        map_linear(
            bootinfo->use_pae,
            bootinfo->page_tables,
            data_offset >> PAGE_BITS,
            NUM_PAGES(bootinfo->data_size),
            bootinfo->data_physaddr | X86_PTE_READ_WRITE | X86_PTE_GLOBAL | X86_PTE_NX
        );
    }

    /* map memory allocations */
    map_linear(
        bootinfo->use_pae,
        bootinfo->page_tables,
        (ALLOC_BASE - JINUE_KLIMIT) >> PAGE_BITS,
        NUM_PAGES(BOOT_SIZE_AT_16MB),
        MEMORY_ADDR_16MB | X86_PTE_READ_WRITE | X86_PTE_GLOBAL | X86_PTE_NX
    );

    /* link page tables in page directory */
    size_t per_table_bits = bootinfo->use_pae ? 9 : 10;

    clear_ptes(bootinfo->use_pae, bootinfo->page_directory, 0, (1 << per_table_bits));

    map_linear(
        bootinfo->use_pae,
        bootinfo->page_directory,
        (bootinfo->use_pae ? 0 : JINUE_KLIMIT) >> (PAGE_BITS + per_table_bits),
        NUM_PAGES(ADDR_4GB - JINUE_KLIMIT) / (1 << per_table_bits),
        (uint32_t)bootinfo->page_tables | X86_PTE_READ_WRITE
    );

    if(!bootinfo->use_pae) {
        return;
    }

    /* link page directory to PDPT */
    uint64_t *pdpt = (uint64_t *)bootinfo->cr3;
    pdpt[0] = 0;
    pdpt[1] = 0;
    pdpt[2] = 0;
    pdpt[3] = (uint32_t)bootinfo->page_directory | X86_PTE_PRESENT;
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
 * alloc_pages() must not be called between this function and the matching call
 * to cleanup_after_paging().
 * 
 * @param bootinfo boot information structure
 */
void prepare_for_paging(bootinfo_t *bootinfo) {
    /* mappings for the kernel image at 0x100000 (1MB) */
    pte_t *page_tables_1mb = alloc_pages(bootinfo, PAGE_SIZE);

    size_t per_table_bits = bootinfo->use_pae ? 9 : 10;

    clear_ptes(bootinfo->use_pae, page_tables_1mb, 0, 1 << per_table_bits);

    map_linear(
        bootinfo->use_pae,
        page_tables_1mb,
        MEMORY_ADDR_1MB >> PAGE_BITS,
        ((char *)bootinfo->image_top - (char *)bootinfo->image_start) >> PAGE_BITS,
        (uint32_t)bootinfo->image_start | X86_PTE_NX
    );

    /* Make sure this setup code is executable.
     *
     * We don't need to do the same for the kernel code segment here because
     * these temporary mappings won't be used for long enough. */
    map_linear(
        bootinfo->use_pae,
        page_tables_1mb,
        MEMORY_ADDR_1MB >> PAGE_BITS,
        1,
        (uint32_t)bootinfo->image_start
    );
    
    /* mappings for the initial memory allocations at 0x1000000 (16MB) */
    pte_t *page_tables_16mb = alloc_pages(
        bootinfo,
        NUM_PAGES(BOOT_SIZE_AT_16MB) / (1 << per_table_bits) * PAGE_SIZE
    );

    clear_ptes(bootinfo->use_pae, page_tables_16mb, 0, NUM_PAGES(BOOT_SIZE_AT_16MB));

    map_linear(
        bootinfo->use_pae,
        page_tables_16mb,
        0,
        NUM_PAGES(BOOT_SIZE_AT_16MB),
        MEMORY_ADDR_16MB | X86_PTE_READ_WRITE | X86_PTE_NX
    );

    /* Link the temporary page tables into the page directory. */
    pte_t *page_directory;

    if(!bootinfo->use_pae) {
        page_directory = bootinfo->page_directory;
    } else {
        page_directory = alloc_pages(bootinfo, PAGE_SIZE);
        clear_ptes(bootinfo->use_pae, page_directory, 0, (1 << per_table_bits));
    }

    map_linear(
        bootinfo->use_pae,
        page_directory,
        MEMORY_ADDR_1MB >> (PAGE_BITS + per_table_bits),
        1,
        (uint32_t)page_tables_1mb | X86_PTE_READ_WRITE
    );

    map_linear(
        bootinfo->use_pae,
        page_directory,
        MEMORY_ADDR_16MB >> (PAGE_BITS + per_table_bits),
        NUM_PAGES(BOOT_SIZE_AT_16MB) / (1 << per_table_bits),
        (uint32_t)page_tables_16mb | X86_PTE_READ_WRITE
    );

    if(bootinfo->use_pae) {
        uint64_t *pdpt = (uint64_t *)bootinfo->cr3;
        pdpt[0] = (uint32_t)page_directory | X86_PTE_PRESENT;
    }

    /* free memory 
     *
     * There must be no call to alloc_pages() until cleanup_after_paging() is
     * called. */
    set_alloc_pages_address(bootinfo, page_tables_1mb);
}

/**
 * Remove the mapping created by prepare_for_paging()
 * 
 * @param bootinfo boot information structure
 */
void cleanup_after_paging(const bootinfo_t *bootinfo) {
    if(bootinfo->use_pae) {
        uint64_t *pdpt = (uint64_t *)bootinfo->cr3;
        pdpt[0] = 0;
        return;
    }

    size_t per_table_bits = 10;

    clear_ptes(
        bootinfo->use_pae,
        bootinfo->page_directory,
        (KERNEL_BASE - BOOT_OFFSET_FROM_1MB) >> (PAGE_BITS + per_table_bits),
        1
    );

    clear_ptes(
        bootinfo->use_pae,
        bootinfo->page_directory,
        MEMORY_ADDR_16MB >> (PAGE_BITS + per_table_bits),
        NUM_PAGES(BOOT_SIZE_AT_16MB) / (1 << per_table_bits)
    );
}
