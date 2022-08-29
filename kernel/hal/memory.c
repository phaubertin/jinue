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

#include <jinue-common/asm/e820.h>
#include <jinue-common/errno.h>
#include <hal/boot.h>
#include <hal/memory.h>
#include <hal/vm.h>
#include <assert.h>
#include <boot.h>
#include <panic.h>
#include <printk.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <util.h>

typedef struct {
    uint64_t    start;
    uint64_t    end;
} memory_range_t;

static uintptr_t *memory_array;

static size_t memory_array_entries;

static bool memory_range_is_within(
        const memory_range_t *enclosed,
        const memory_range_t *enclosing) {

    return enclosed->start >= enclosing->start && enclosed->end <= enclosed->end;
}

static bool memory_ranges_overlap(
        const memory_range_t *range1,
        const memory_range_t *range2) {

    return !(range1->end <= range2->start || range1->start >= range2->end);
}

static bool range_is_in_available_memory(
        const memory_range_t    *range,
        const boot_info_t       *boot_info) {

    bool retval = false;

    for(int idx = 0; idx < boot_info->e820_entries; ++idx) {
        memory_range_t entry_range;

        const e820_t *entry = &boot_info->e820_map[idx];
        entry_range.start   = entry->addr;
        entry_range.end     = entry->addr + entry->size;

        if(entry->type == E820_RAM) {
            if(memory_range_is_within(range, &entry_range)) {
                retval = true;
            }
        }
        else {
            if(memory_ranges_overlap(range, &entry_range)) {
                return false;
            }
        }
    }

    return retval;
}

/**
 * Check the system has sufficient memory to complete kernel initialization.
 *
 * We need:
 * - One MB at 0x100000 (i.e. at address 1MB). This is where the kernel image
 *   is initially loaded by the boot loader and some of that memory is used
 *   during early boot as well, for the initial boot stack and heap and initial
 *   page tables among other things. All memory in this range is freed at the
 *   end of kernel initialization.
 * - BOOT_SIZE_AT_16MB at 0x1000000 (i.e. at address 16MB). The kernel image is
 *   moved there during kernel initializations and all permanent page
 *   allocations during kernel initialization come from this range. At the end
 *   of kernel initialization, remaining memory in this range is used to
 *   initialize the kernel's page allocator.
 *
 * This function checks the BIOS memory map to ensure these two memory regions
 * are completely within available memory and do not intersect any reserved
 * range. It also does the same check on the initial RAM disk loaded by the
 * boot loader.
 *
 * If any of these checks fail, the result is a kernel panic.
 *
 * @param boot_info boot information structure
 *
 * */
void check_memory(const boot_info_t *boot_info) {
    memory_range_t range_at_1mb = {
            .start  = MEMORY_ADDR_1MB,
            .end    = MEMORY_ADDR_1MB + 1 * MB
    };
    memory_range_t range_at_16mb = {
            .start  = MEMORY_ADDR_16MB,
            .end    = MEMORY_ADDR_16MB + BOOT_SIZE_AT_16MB
    };

    if(! range_is_in_available_memory(&range_at_16mb, boot_info)) {
        panic("Insufficient or no memory at 0x1000000 (i.e. at 16MB)");
    }

    if(! range_is_in_available_memory(&range_at_1mb, boot_info)) {
        panic("Insufficient or no memory at 0x100000 (i.e. at 1MB)");
    }

    if(boot_info->ramdisk_start) {
        memory_range_t ramdisk_range;
        ramdisk_range.start = boot_info->ramdisk_start;
        ramdisk_range.end   = boot_info->ramdisk_start + boot_info->ramdisk_size;

        if(! range_is_in_available_memory(&ramdisk_range, boot_info)) {
            panic("Initial RAM disk was loaded in unavailable or reserved memory");
        }

        if(boot_info->ramdisk_start < range_at_16mb.end) {
            panic("Initial RAM disk was loaded in memory reserved for the kernel");
        }
    }
}

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
 * @param boot_info boot information structure
 * @return top of memory usable by kernel
 * */
static uint64_t memory_find_top(const boot_info_t *boot_info) {
    uint64_t memory_top = 0;

    for(int idx = 0; idx < boot_info->e820_entries; ++idx) {
        const e820_t *entry = &boot_info->e820_map[idx];

        /* Only consider available memory entries. */
        if(entry->type != E820_RAM) {
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

    printk("Top memory address for kernel is 0x%q\n", memory_top);

    return memory_top;
}

/**
 * Initialize the array used by memory_lookup_page()
 *
 * @param boot_info boot information structure
 * @param boot_alloc the boot allocator state
 *
 * */
void memory_initialize_array(
        boot_alloc_t        *boot_alloc,
        const boot_info_t   *boot_info) {

    const size_t entries_per_page = PAGE_SIZE / sizeof(uintptr_t);

    const uint64_t memory_top   = memory_find_top(boot_info);
    const size_t num_pages      = memory_top / PAGE_SIZE;
    const size_t array_entries  = ALIGN_END(num_pages, entries_per_page);
    const size_t array_pages    = array_entries / entries_per_page;

    uintptr_t *array = boot_page_alloc_n(boot_alloc, array_pages);

    for(    uint32_t addr = MEMORY_ADDR_16MB;
            addr < MEMORY_ADDR_16MB + BOOT_SIZE_AT_16MB;
            addr += PAGE_SIZE) {

        array[addr / PAGE_SIZE] = PHYS_TO_VIRT_AT_16MB(addr);
    }

    memory_array            = (uintptr_t *)PHYS_TO_VIRT_AT_16MB(array);
    memory_array_entries    = array_entries;
}

/**
 * Lookup the virtual address of a page frame mapped by the kernel
 *
 * Must only be used for memory owned by the kernel, not for userspace-owned
 * memory. Every page frame owned by the kernel is mapped at exactly one
 * address in the kernel's address space (i.e. somewhere above KLIMIT).
 *
 * */
void *memory_lookup_page(uint64_t paddr) {
    uint64_t entry_index = paddr / PAGE_SIZE;

    if(entry_index >= memory_array_entries) {
        return NULL;
    }

    return (void *)memory_array[entry_index];
}

int memory_get_map(jinue_mem_map_t *map, size_t buffer_size) {
    unsigned int idx;

    const boot_info_t *boot_info = get_boot_info();

    if(buffer_size < sizeof(jinue_mem_map_t) + boot_info->e820_entries * sizeof(jinue_mem_entry_t) ) {
        return -JINUE_EINVAL;
    }

    map->num_entries = boot_info->e820_entries;

    for(idx = 0; idx < map->num_entries; ++idx) {
        map->entry[idx].addr = boot_info->e820_map[idx].addr;
        map->entry[idx].size = boot_info->e820_map[idx].size;
        map->entry[idx].type = boot_info->e820_map[idx].type;
    }

    return 0;
}
