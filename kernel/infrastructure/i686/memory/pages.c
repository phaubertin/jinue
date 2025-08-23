/*
 * Copyright (C) 2019-2025 Philippe Aubertin.
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

#include <kernel/domain/services/logging.h>
#include <kernel/infrastructure/acpi/asm/addrmap.h>
#include <kernel/infrastructure/i686/memory/pages.h>
#include <kernel/infrastructure/i686/pmap/pmap.h>
#include <kernel/infrastructure/i686/boot_alloc.h>
#include <kernel/utils/utils.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>

static struct {
    uintptr_t   *array;
    size_t       size;
} page_frames;

/**
 * Find the top of memory usable by the kernel
 *
 * Checks the BIOS memory map for the top of the highest range of available
 * memory under the 4GB mark (i.e. address 0x100000000).
 *
 * The kernel can only use the first 4GB of memory on 32-bit x86, even with PAE
 * enabled. This is because the architecture requires PDPTs to be in the first
 * 4GB (CR3 is only 32 bits) and we don't want to have to deal with the
 * complexity of having to allocate in the first 4GB only for specific
 * allocations.
 *
 * @param bootinfo boot information structure
 * @return top of memory usable by kernel
 */
static uint64_t memory_find_top(const bootinfo_t *bootinfo) {
    uint64_t memory_top = 0;

    for(int idx = 0; idx < bootinfo->addr_map_entries; ++idx) {
        const acpi_addr_range_t *entry = &bootinfo->acpi_addr_map[idx];

        /* Only consider available memory entries. */
        if(entry->type != ACPI_ADDR_RANGE_MEMORY) {
            continue;
        }

        /* The kernel can only use the first 4GB of memory. */
        if(entry->addr >= ADDR_4GB) {
            continue;
        }

        uint64_t entry_top = ALIGN_START(entry->addr + entry->size, (uint64_t)PAGE_SIZE);

        if(entry_top >= ADDR_4GB) {
            /* ADDR_4GB is correctly aligned. */
            entry_top = ADDR_4GB;
        }

        if(entry_top > memory_top) {
            memory_top = entry_top;
        }
    }

    info("Top memory address for kernel is %#" PRIx64, memory_top);

    return memory_top;
}

/**
 * Initialize the array used by memory_lookup_page()
 *
 * @param bootinfo boot information structure
 * @param boot_alloc the boot allocator state
 */
void memory_initialize_array(
        boot_alloc_t        *boot_alloc,
        const bootinfo_t    *bootinfo) {

    const size_t entries_per_page = PAGE_SIZE / sizeof(uintptr_t);

    const uint64_t memory_top   = memory_find_top(bootinfo);
    const size_t num_pages      = NUM_PAGES(memory_top);
    const size_t array_entries  = ALIGN_END(num_pages, entries_per_page);
    const size_t array_pages    = array_entries / entries_per_page;

    page_frames.array           = boot_page_alloc_n(boot_alloc, array_pages);
    page_frames.size            = array_entries;

    uint32_t top_at_16mb        = MEMORY_ADDR_16MB + BOOT_SIZE_AT_16MB;

    for(uint32_t addr = MEMORY_ADDR_16MB; addr < top_at_16mb; addr += PAGE_SIZE) {
        page_frames.array[page_number_of(addr)] = PHYS_TO_VIRT_AT_16MB(addr);
    }
}

/**
 * Lookup the virtual address of a page frame mapped by the kernel
 *
 * Must only be used for memory owned by the kernel, not for userspace-owned
 * memory. Every page frame owned by the kernel is mapped at exactly one
 * address in the kernel's address space (i.e. somewhere above JINUE_KLIMIT).
 */
void *memory_lookup_page(uint64_t paddr) {
    uint64_t entry_index = PAGE_NUMBER(paddr);

    if(entry_index >= page_frames.size) {
        return NULL;
    }

    return (void *)page_frames.array[entry_index];
}
