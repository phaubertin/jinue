/*
 * Copyright (C) 2019-2023 Philippe Aubertin.
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

#include <jinue/shared/asm/errno.h>
#include <kernel/domain/services/logging.h>
#include <kernel/domain/services/panic.h>
#include <kernel/infrastructure/acpi/asm/addrmap.h>
#include <kernel/infrastructure/acpi/types.h>
#include <kernel/infrastructure/i686/pmap/pmap.h>
#include <kernel/infrastructure/i686/boot_alloc.h>
#include <kernel/infrastructure/i686/memory.h>
#include <kernel/interface/i686/boot.h>
#include <kernel/machine/memory.h>
#include <kernel/utils/utils.h>
#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint64_t    start;
    uint64_t    end;
} memory_range_t;

static uintptr_t *memory_array;

static size_t memory_array_entries;

static bool memory_range_is_within(
        const memory_range_t *enclosed,
        const memory_range_t *enclosing) {

    return enclosed->start >= enclosing->start && enclosed->end <= enclosing->end;
}

static bool memory_ranges_overlap(
        const memory_range_t *range1,
        const memory_range_t *range2) {

    return !(range1->end <= range2->start || range1->start >= range2->end);
}

static bool range_is_in_available_memory(
        const memory_range_t    *range,
        const bootinfo_t        *bootinfo) {

    bool retval = false;

    for(int idx = 0; idx < bootinfo->addr_map_entries; ++idx) {
        memory_range_t entry_range;

        const acpi_addr_range_t *entry  = &bootinfo->acpi_addr_map[idx];
        entry_range.start               = entry->addr;
        entry_range.end                 = entry->addr + entry->size;

        if(entry->type == ACPI_ADDR_RANGE_MEMORY) {
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
 * @param bootinfo boot information structure
 *
 * */
void check_memory(const bootinfo_t *bootinfo) {
    const memory_range_t range_at_1mb = {
            .start  = MEMORY_ADDR_1MB,
            .end    = MEMORY_ADDR_1MB + 1 * MB
    };
    const memory_range_t range_at_16mb = {
            .start  = MEMORY_ADDR_16MB,
            .end    = MEMORY_ADDR_16MB + BOOT_SIZE_AT_16MB
    };

    if(! range_is_in_available_memory(&range_at_16mb, bootinfo)) {
        panic("Insufficient or no memory at 0x1000000 (i.e. at 16MB)");
    }

    if(! range_is_in_available_memory(&range_at_1mb, bootinfo)) {
        panic("Insufficient or no memory at 0x100000 (i.e. at 1MB)");
    }

    if(bootinfo->ramdisk_start) {
        memory_range_t ramdisk_range;
        ramdisk_range.start = bootinfo->ramdisk_start;
        ramdisk_range.end   = bootinfo->ramdisk_start + bootinfo->ramdisk_size;

        if(! range_is_in_available_memory(&ramdisk_range, bootinfo)) {
            panic("Initial RAM disk was loaded in unavailable or reserved memory");
        }

        if(bootinfo->ramdisk_start < range_at_16mb.end) {
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
 * @param bootinfo boot information structure
 * @return top of memory usable by kernel
 * */
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
 *
 * */
void memory_initialize_array(
        boot_alloc_t        *boot_alloc,
        const bootinfo_t    *bootinfo) {

    const size_t entries_per_page = PAGE_SIZE / sizeof(uintptr_t);

    const uint64_t memory_top   = memory_find_top(bootinfo);
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

static int map_memory_type(const acpi_addr_range_t *addr_range) {
    /* The values of the JINUE_MEMYPE_... constants are based on the ACPI
     * address range types, i.e. all non OEM-defined values are the same
     * between both.
     * 
     * We reserve the OEM defined range from 0xf0000000 for Jinue-specific
     * values, so we fold all OEM defined values from the system address map
     * into a single value that means "OEM defined". */
    if(addr_range->type >= ACPI_ADDR_RANGE_OEM_START) {
        return JINUE_MEMYPE_OEM;
    }

    switch(addr_range->type) {
    case ACPI_ADDR_RANGE_MEMORY:
    case ACPI_ADDR_RANGE_RESERVED:
    case ACPI_ADDR_RANGE_ACPI:
    case ACPI_ADDR_RANGE_NVS:
    case ACPI_ADDR_RANGE_UNUSABLE:
    case ACPI_ADDR_RANGE_DISABLED:
    case ACPI_ADDR_RANGE_PERSISTENT:
    case ACPI_ADDR_RANGE_OEM:
        /* ACPI address range types and Jinue memory type have the same value
         * for these types. */
        return addr_range->type;
    default:
        /* The ACPI specification states that any undefined type value should
         * be treated as reserved. */
        return JINUE_MEMYPE_RESERVED;
    }
}

static void align_range(memory_range_t *dest, bool is_available) {
    if(is_available) {
        /* shrink to align ends to page boundaries */
        dest->start = ALIGN_END(dest->start, PAGE_SIZE);
        dest->end   = ALIGN_START(dest->end, PAGE_SIZE);
    }
    else {
        /* grow to align ends to page boundaries */
        dest->start = ALIGN_START(dest->start, PAGE_SIZE);
        dest->end   = ALIGN_END(dest->end, PAGE_SIZE);
    }
}

static void assign_and_align_entry(memory_range_t *dest, const acpi_addr_range_t *entry) {
    dest->start = entry->addr;
    dest->end   = entry->addr + entry->size;
    align_range(dest, entry->type == ACPI_ADDR_RANGE_MEMORY);
}

static void clip_memory_range(memory_range_t *dest, const memory_range_t *clipping) {
    if(clipping->start <= dest->start) {
        if(clipping->end <= dest->start) {
            return;
        }

        dest->start = clipping->end;

        if(dest->end < dest->start) {
            dest->end = dest->start;
        }

        return;
    }

    if(clipping->start >= dest->end) {
        return;
    }

    if(clipping->end >= dest->end) {
        dest->end = clipping->start;
        return;
    }

    const size_t low_size = clipping->start - dest->start;
    const size_t high_size = dest->end - clipping->end;

    if(high_size > low_size) {
        dest->start = clipping->end;
        return;
    }

    dest->end = clipping->start;
}

static void clip_available_range(memory_range_t *dest, const bootinfo_t *bootinfo) {
    for(int idx = 0; idx < bootinfo->addr_map_entries; ++idx) {
        const acpi_addr_range_t *entry = &bootinfo->acpi_addr_map[idx];

        if(entry->type == ACPI_ADDR_RANGE_MEMORY) {
            continue;
        }

        memory_range_t not_available;
        assign_and_align_entry(&not_available, entry);
        clip_memory_range(dest, &not_available);
    }

    memory_range_t ramdisk = {
        .start  = bootinfo->ramdisk_start,
        .end    = bootinfo->ramdisk_start + bootinfo->ramdisk_size
    };
    align_range(&ramdisk, false);
    clip_memory_range(dest, &ramdisk);
}

static void find_range_for_loader(memory_range_t *dest, const bootinfo_t *bootinfo) {
    /* First, find the largest available range over the 4GB mark. */
    memory_range_t largest_over_4gb = {0};

    for(int idx = 0; idx < bootinfo->addr_map_entries; ++idx) {
        const acpi_addr_range_t *entry = &bootinfo->acpi_addr_map[idx];

        if(entry->type != ACPI_ADDR_RANGE_MEMORY) {
            continue;
        }

        if(entry->addr < UINT64_C(4) * GB) {
            continue;
        }

        memory_range_t available;
        assign_and_align_entry(&available, entry);
        clip_available_range(&available, bootinfo);

        uint64_t available_size = available.end - available.start;
        uint64_t largest_size = largest_over_4gb.end - largest_over_4gb.start;

        if(available_size > largest_size) {
            largest_over_4gb = available;
        }
    }

    /* Then, compare this to the region just above the kernel data. */
    memory_range_t under_4gb = {0};

    for(int idx = 0; idx < bootinfo->addr_map_entries; ++idx) {
        const acpi_addr_range_t *entry = &bootinfo->acpi_addr_map[idx];

        if(entry->type != ACPI_ADDR_RANGE_MEMORY) {
            continue;
        }

        uint64_t start = MEMORY_ADDR_16MB + BOOT_SIZE_AT_16MB;

        if(entry->addr + entry->size <= start || entry->addr > MEMORY_ADDR_16MB) {
            continue;
        }

        under_4gb.start = start;
        under_4gb.end   = entry->addr + entry->size;
        align_range(&under_4gb, true);
        clip_available_range(&under_4gb, bootinfo);
        break;
    }

    uint64_t under_4gb_size = under_4gb.end - under_4gb.start;
    uint64_t over_4gb_size = largest_over_4gb.end - largest_over_4gb.start;

    if(under_4gb_size > over_4gb_size) {
        *dest = under_4gb;
    }
    else {
        *dest = largest_over_4gb;
    }
}

int machine_get_address_map(const jinue_buffer_t *buffer) {
    const bootinfo_t *bootinfo = get_bootinfo();

    const uintptr_t kernel_image_size =
            (uintptr_t)bootinfo->image_top - (uintptr_t)bootinfo->image_start;

    memory_range_t loader_range;
    find_range_for_loader(&loader_range, bootinfo);

    const jinue_addr_map_entry_t kernel_regions[] = {
        {
            .addr = bootinfo->ramdisk_start,
            .size = bootinfo->ramdisk_size,
            .type = JINUE_MEMYPE_RAMDISK
        },
        {
            .addr = VIRT_TO_PHYS_AT_16MB(bootinfo->image_start),
            .size = kernel_image_size,
            .type = JINUE_MEMYPE_KERNEL_IMAGE
        },
        {
            .addr = VIRT_TO_PHYS_AT_16MB(bootinfo->image_top),
            .size = BOOT_SIZE_AT_16MB - kernel_image_size,
            .type = JINUE_MEMYPE_KERNEL_RESERVED
        },
        {
            .addr = loader_range.start,
            .size = loader_range.end - loader_range.start,
            .type = JINUE_MEMYPE_LOADER_AVAILABLE
        }
    };

    const size_t addr_map_entries       = bootinfo->addr_map_entries;
    const size_t kernel_entries     = sizeof(kernel_regions) / sizeof(kernel_regions[0]);
    const size_t total_entries      = addr_map_entries + kernel_entries;
    const size_t result_size        =
            sizeof(jinue_addr_map_t) + total_entries * sizeof(jinue_addr_map_entry_t);

    jinue_addr_map_t *map = buffer->addr;

    if(buffer->size >= sizeof(jinue_addr_map_t)) {
        map->num_entries = total_entries;
    }

    if(buffer->size < result_size) {
        return -JINUE_E2BIG;
    }

    for(unsigned int idx = 0; idx < addr_map_entries; ++idx) {
        const acpi_addr_range_t *addr_range = &bootinfo->acpi_addr_map[idx];
        map->entry[idx].addr = addr_range->addr;
        map->entry[idx].size = addr_range->size;
        map->entry[idx].type = map_memory_type(addr_range);
    }

    for(unsigned int idx = 0; idx < kernel_entries; ++idx) {
        map->entry[addr_map_entries + idx] = kernel_regions[idx];
    }

    return 0;
}
