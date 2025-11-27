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

#include <jinue/shared/asm/errno.h>
#include <kernel/domain/services/panic.h>
#include <kernel/infrastructure/acpi/asm/addrmap.h>
#include <kernel/infrastructure/i686/memory/addrmap.h>
#include <kernel/infrastructure/i686/pmap/pmap.h>
#include <kernel/interface/i686/boot.h>
#include <kernel/machine/memory.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint64_t    start;
    uint64_t    end;
} memory_range_t;

static struct {
    jinue_addr_map_entry_t  map[16];
    size_t                  num_entries;
} kernel_addrmap = {0};

/**
 * Determines whether a memory range is completely contained within another
 *
 * @param enclosed range expected to be within the other
 * @param enclosing range expected to contain the other
 * @return true if enclosed is completely within enclosing, false otherwise
 */
static bool range_is_within(
        const memory_range_t *enclosed,
        const memory_range_t *enclosing) {

    return enclosed->start >= enclosing->start && enclosed->end <= enclosing->end;
}

/**
 * Determines whether two memory ranges intersect
 *
 * @param range1 first range
 * @param range2 second range
 * @return true if both ranges intersect, false otherwise
 */
static bool ranges_intersect(
        const memory_range_t *range1,
        const memory_range_t *range2) {

    return !(range1->end <= range2->start || range1->start >= range2->end);
}

/**
 * Determines whether a memory range is in available memory
 * 
 * A range is in available memory if it is completely contained in an available
 * entry of the ACPI address map and if it intersects no unavailable entry.
 *
 * @param range memory range
 * @param bootinfo boot information structure (for the address map)
 * @return true if range is in available memory, false otherwise
 */
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
            if(range_is_within(range, &entry_range)) {
                retval = true;
            }
        }
        else {
            if(ranges_intersect(range, &entry_range)) {
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
 */
void check_system_address_map(const bootinfo_t *bootinfo) {
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
 * Map ACPI address map entry types to Jinue memory types
 * 
 * The values of the JINUE_MEMYPE_... constants are based on the ACPI address
 * range types, i.e. all non OEM-defined values are the same.
 * 
 * We reserve the OEM defined range starting at 0xf0000000 for Jinue-specific
 * values, so we fold all OEM defined values from the ACPI address map into a
 * single value that means "OEM defined".
 *
 * @param bootinfo boot information structure
 */
static int map_memory_type(const acpi_addr_range_t *addr_range) {
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

/**
 * Page-align an available range by shrinking to nearest page boundaries
 *
 * @param range memory range to align
 */
static void page_align_available_range(memory_range_t *range) {
    range->start    = ALIGN_END(range->start, PAGE_SIZE);
    range->end      = ALIGN_START(range->end, PAGE_SIZE);
}

/**
 * Page-align an unavailable range by growing to nearest page boundaries
 *
 * @param range memory range to align
 */
static void page_align_unavailable_range(memory_range_t *range) {
    range->start    = ALIGN_START(range->start, PAGE_SIZE);
    range->end      = ALIGN_END(range->end, PAGE_SIZE);
}

/**
 * Page-align an unavailable map entry by growing to nearest page boundaries
 *
 * @param entry address map entry align
 */
static void page_aligned_unavailable_entry(jinue_addr_map_entry_t *entry) {
    memory_range_t range;

    range.start = entry->addr;
    range.end   = entry->addr + entry->size;
    page_align_unavailable_range(&range);

    entry->addr = range.start;
    entry->size = range.end - range.start;
}

/**
 * Assign and align an entry from the ACPI address map to a range
 * 
 * The range is aligned in the correct direction, i.e. by growing or shrinking,
 * based on the entry type.
 *
 * @param dest memory range to assign
 * @param entry ACPI address map entry
 */
static void assign_and_align_from_acpi(memory_range_t *dest, const acpi_addr_range_t *entry) {
    dest->start = entry->addr;
    dest->end   = entry->addr + entry->size;

    if(entry->type == ACPI_ADDR_RANGE_MEMORY) {
        page_align_available_range(dest);
    }
    else {
        page_align_unavailable_range(dest);
    }
}

/**
 * Clip a destination range so it doesn't intersect a clipping range
 * 
 * @param dest destination range
 * @param clipping clipping range
 */
static void clip_range(memory_range_t *dest, const memory_range_t *clipping) {
    /* There is nothing to clip if the clipping range does not intersect the
     * destination range. */
    if(! ranges_intersect(dest, clipping)) {
        return;
    }

    /* The clipping range starts first (and we know the ranges intersect). */
    if(clipping->start <= dest->start) {
        /* Clip the end of the destination range. */
        dest->start = clipping->end;

        /* If the clipping range starts first and ends last, it means the
         * destination range is completely within the clipping range. In that
         * case, the size of the destination range is zero. */
        if(dest->end < dest->start) {
            dest->end = dest->start;
        }

        return;
    }

    /* The ends last (and we know the ranges intersect). */
    if(clipping->end >= dest->end) {
        /* Clip the start of the destination range. */
        dest->end = clipping->start;
        return;
    }

    /* Here, the destination range starts first and ends last, which means it
     * is split in two by the clipping range. Let's keep the biggest chunk. */
    const size_t low_size = clipping->start - dest->start;
    const size_t high_size = dest->end - clipping->end;

    if(high_size > low_size) {
        dest->start = clipping->end;
        return;
    }

    dest->end = clipping->start;
}

/**
 * Clip an available memory range so it doesn't intersect unavailable ranges
 * 
 * @param dest destination available memory range
 * @param bootinfo boot information structure
 */
static void clip_available_range(memory_range_t *dest, const bootinfo_t *bootinfo) {
    for(int idx = 0; idx < bootinfo->addr_map_entries; ++idx) {
        const acpi_addr_range_t *entry = &bootinfo->acpi_addr_map[idx];

        if(entry->type == ACPI_ADDR_RANGE_MEMORY) {
            continue;
        }

        memory_range_t not_available;
        assign_and_align_from_acpi(&not_available, entry);
        clip_range(dest, &not_available);
    }

    memory_range_t ramdisk = {
        .start  = bootinfo->ramdisk_start,
        .end    = bootinfo->ramdisk_start + bootinfo->ramdisk_size
    };
    page_align_unavailable_range(&ramdisk);
    clip_range(dest, &ramdisk);
}

/**
 * Find a range that the user space loader can use for allocations
 * 
 * This is a hint provided to the user space loader so it can start allocating
 * memory early without having to parse the full address map.
 *
 * @param dest memory range to assign
 * @param bootinfo boot information structure (for the address map)
 */
static void find_available_range_for_loader(memory_range_t *dest, const bootinfo_t *bootinfo) {
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
        assign_and_align_from_acpi(&available, entry);
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
        page_align_available_range(&under_4gb);
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

/**
 * Add a kernel entry to the address map reported to user space.
 * 
 * @param entry kernel entry to add
 */
static void add_kernel_entry(const jinue_addr_map_entry_t *entry) {
    kernel_addrmap.map[kernel_addrmap.num_entries] = *entry;
    ++kernel_addrmap.num_entries;
}

/**
 * Deduplicate and merge a shared address map entry being added
 * 
 * Given a new shared entry to be added, this function:
 * 
 * 1) Attempts to find an existing shared entry that intersects the new one
 *    and, if one is found, extend it to include the range of the new entry.
 * 2) Determine whether the new entry should be a added or not to the address
 *    map. It isn't added if it is redundant with an existing entry.
 * 
 * This function does not handle all duplication cases. Specifically, a new
 * entry could make it possible to merge two existing entries by bridging them,
 * but this case, which would require deleting an existing entry, is not
 * handled. However, it works well enough to handle commonly encountered cases,
 * notably ACPI tables which are often multiple structures smaller than a page
 * packed closely together, with multiple sharing the same page.
 * 
 * @param new_entry the new entry being considered
 * @return true if the entry should be added, false if is shouldn't
 */
static bool deduplicate_shared_entry(const jinue_addr_map_entry_t *new_entry) {
    memory_range_t new_range;

    new_range.start = new_entry->addr;
    new_range.end   = new_entry->addr + new_entry->size;

    for(int idx = 0; idx < kernel_addrmap.num_entries; ++idx) {
        jinue_addr_map_entry_t *existing = &kernel_addrmap.map[idx];

        if(existing->type != JINUE_MEMYPE_KERNEL_SHARED) {
            continue;
        }

        memory_range_t  existing_range;
        
        existing_range.start    = existing->addr;
        existing_range.end      = existing->addr + existing->size;

        if(!ranges_intersect(&existing_range, &new_range)) {
            continue;
        }

        if(new_range.start < existing_range.start) {
            existing_range.start = new_range.start;
        }

        if(new_range.end > existing_range.end) {
            existing_range.end = new_range.end;
        }

        existing->addr = existing_range.start;
        existing->size = existing_range.end - existing_range.start;

        return false;
    }

    return true;
}

/**
 * Add a shared memory range to the address map.
 * 
 * Memory in shared ranges can be mapped read only by user space.
 * 
 * This function takes care of page-aligning the range and deduplicating
 * entries, so it can be used for e.g. individual ACPI tables.
 * 
 * @param addr start address of the range
 * @param size size of the range in bytes
 */
void machine_add_shared_to_address_map(uint64_t addr, uint64_t size) {
    jinue_addr_map_entry_t entry;

    entry.type = JINUE_MEMYPE_KERNEL_SHARED;
    entry.addr = addr;
    entry.size = size;
    page_aligned_unavailable_entry(&entry);

    if(!deduplicate_shared_entry(&entry)) {
        return;
    }

    add_kernel_entry(&entry);
}

/**
 * Initialize the address map reported to user space
 * 
 * @param bootinfo boot information structure
 */
void initialize_address_map(const bootinfo_t *bootinfo) {
    jinue_addr_map_entry_t entry;

    entry.type = JINUE_MEMYPE_KERNEL_RESERVED;
    entry.addr = MEMORY_ADDR_16MB;
    entry.size = BOOT_SIZE_AT_16MB;
    add_kernel_entry(&entry);

    entry.type = JINUE_MEMYPE_RAMDISK;
    entry.addr = bootinfo->ramdisk_start;
    entry.size = bootinfo->ramdisk_size;
    machine_add_shared_to_address_map(entry.addr, entry.size);
    add_kernel_entry(&entry);

    entry.type = JINUE_MEMYPE_KERNEL_IMAGE;
    entry.addr = VIRT_TO_PHYS_AT_16MB(bootinfo->image_start);
    entry.size = (uintptr_t)bootinfo->image_top - (uintptr_t)bootinfo->image_start;
    machine_add_shared_to_address_map(entry.addr, entry.size);
    add_kernel_entry(&entry);

    memory_range_t loader_range;
    find_available_range_for_loader(&loader_range, bootinfo);

    entry.type = JINUE_MEMYPE_LOADER_AVAILABLE;
    entry.addr = loader_range.start;
    entry.size = loader_range.end - loader_range.start;
    add_kernel_entry(&entry);
}

/**
 * Write the address map for user space to the specified buffer
 * 
 * The written address map is the ACPI address map to which a few
 * Jinue-specific ranges are added:
 * 
 * - The location of the kernel image and RAM disk image.
 * - Memory reserved by the kernel for its own use.
 * - The allocation hint for the user space loader.
 *
 * @param buffer buffer where address map is written
 * @return zero on success, negated error number on error
 */
int machine_get_address_map(const jinue_buffer_t *buffer) {
    const bootinfo_t *bootinfo = get_bootinfo();

    const size_t addr_map_entries   = bootinfo->addr_map_entries;
    const size_t total_entries      = addr_map_entries + kernel_addrmap.num_entries;
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

    for(unsigned int idx = 0; idx < kernel_addrmap.num_entries; ++idx) {
        map->entry[addr_map_entries + idx] = kernel_addrmap.map[idx];
    }

    return 0;
}
