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
#include <hal/memory.h>
#include <hal/vm.h>
#include <assert.h>
#include <panic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint64_t    start;
    uint64_t    end;
} memory_range_t;

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
 *   of the kernel initialization, remaining memory in this range is used to
 *   initialize the kernel's page allocator.
 *
 * TODO remove the next paragraph once the kernel is moved at 16MB
 *
 * Moving the kernel at 0x1000000 (i.e. at 16MB) is not yet implemented. For
 * now, only the first range is in use and allocations made there are permanent.
 *
 * This function checks the BIOS memory map to ensure these two memory regions
 * are completely within available memory and do not intersect any reserved
 * range. It also does the same check on the initial RAM disk loaded by the
 * boot loader.
 *
 * If any of these fails, it results in a kernel panic.
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
