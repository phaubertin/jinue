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

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <jinue-common/asm/e820.h>
#include <hal/kernel.h>
#include <hal/mem.h>
#include <hal/vm.h>
#include <panic.h>


static uint32_t clip_e820_entry_end(const e820_t *entry) {
    /* Clip end address to MEM_ZONE_MEM32_END as the kernel cannot use anything
     * past that. */
    if(entry->addr + entry->size > MEM_ZONE_MEM32_END) {
        return MEM_ZONE_MEM32_END;
    }
    else {
        return entry->addr + entry->size;
    }
}

static bool ranges_overlap(uint32_t range1_start, uint32_t range1_end, uint32_t range2_start, uint32_t range2_end) {
    return range2_start < range1_end && range1_start < range2_end;
}

void mem_check_memory(const boot_info_t *boot_info) {
    int idx;

#if 0
    if((uint64_t)boot_info->ramdisk_start + boot_info->ramdisk_size > MEM_ZONE_MEM32_END) {
        panic("Initial RAM disk loaded too high in memory.");
    }

    uint32_t ramdisk_end = boot_info->ramdisk_start + boot_info->ramdisk_size;
#endif

    /* -------------------------------------------------------------------------
     * We consult the memory map provided by the BIOS to figure out how much
     * memory is available in both zones usable by the kernel. We also want to
     * make sure the initial RAM disk image is in available RAM.
     *
     * The first step in accomplishing this is to iterate over all entries that
     * are reported as available RAM and confirm at least one of them covers
     * each of both zones (at least the start) and the initial RAM disk.
     * ---------------------------------------------------------------------- */
    uint32_t zone_dma16_top = 0;
    uint32_t zone_mem32_top = 0;
#if 0
    bool ramdisk_ok         = false;
#endif

    /** ASSERTION: Any unsigned value less than MEM_ZONE_MEM32_END can be stored in 32 bits. */
    assert(MEM_ZONE_MEM32_END < (uint64_t)4 * GB);

    for(idx = 0; idx < boot_info->e820_entries; ++idx) {
        const e820_t *entry = &boot_info->e820_map[idx];

        /* Consider only usable RAM entries. */
        if(entry->type != E820_RAM) {
            continue;
        }

        /* Ignore entries that start past MEM_ZONE_MEM32_END since the kernel
         * cannot use them. Past this check, entry->addr is assumed to be
         * representable in 32 bits.  */
        if(entry->addr >= MEM_ZONE_MEM32_END) {
            continue;
        }

        uint32_t entry_end = clip_e820_entry_end(entry);

        /* If this entry covers the start the DMA16 zone, adjust the top pointer
         * accordingly. Overlapping entries are resolved in favor of the largest
         * entry. */
        if(entry->addr <= MEM_ZONE_DMA16_START && entry_end > MEM_ZONE_DMA16_START) {
            /* This condition covers the initial case where zone_dma16_top is zero. */
            if(entry_end > zone_dma16_top) {
                zone_dma16_top = entry_end;
            }
        }

        /* Do the same for the MEM32 zone. */
        if(entry->addr <= MEM_ZONE_MEM32_START && entry_end > MEM_ZONE_MEM32_START) {
            if(entry_end > zone_mem32_top) {
                zone_mem32_top = entry_end;
            }
        }

        /* If this entry covers the initial RAM disk, this is good argument in
         * favor of it being in available RAM (one more check below). Unlike the
         * above, the entry must cover the initial RAM disk image completely,
         * not just the start. */
#if 0
        if(entry->addr <= boot_info->ramdisk_start && entry_end >= ramdisk_end) {
            ramdisk_ok = true;
        }
#endif
    }

    /* -------------------------------------------------------------------------
     * Next, iterate over non-available RAM entries of the map to ensure nothing
     * is relevant there.
     * ---------------------------------------------------------------------- */
    for(idx = 0; idx < boot_info->e820_entries; ++idx) {
        const e820_t *entry = &boot_info->e820_map[idx];

        /* Consider only non-usable RAM entries. */
        if(entry->type == E820_RAM) {
            continue;
        }

        if(entry->addr >= MEM_ZONE_MEM32_END) {
            continue;
        }

        uint32_t entry_end = clip_e820_entry_end(entry);

        if(ranges_overlap(MEM_ZONE_DMA16_START, MEM_ZONE_DMA16_END, entry->addr, entry_end)) {
            if(entry->addr > MEM_ZONE_DMA16_START) {
                if(entry->addr < zone_dma16_top) {
                    zone_dma16_top = entry->addr;
                }
            }
            else {
                /* This reserved entry covers the start of the zone. */
                zone_dma16_top = 0;
            }
        }

        if(ranges_overlap(MEM_ZONE_MEM32_START, MEM_ZONE_MEM32_END, entry->addr, entry_end)) {
            if(entry->addr > MEM_ZONE_MEM32_START) {
                if(entry->addr < zone_mem32_top) {
                    zone_mem32_top = entry->addr;
                }
            }
            else {
                zone_mem32_top = 0;
            }
        }

        /* Check for overlap with the initial RAM disk. */
#if 0
        if(ranges_overlap(boot_info->ramdisk_start, ramdisk_end, entry->addr, entry_end)) {
            ramdisk_ok = false;
        }
#endif
    }

    /* -------------------------------------------------------------------------
     * Now that we are done, let's look at the results.
     * ---------------------------------------------------------------------- */
#if 0
    if(! ramdisk_ok) {
        panic("Initial RAM disk was loaded in reserved memory.");
    }
#endif

    if(zone_dma16_top < EARLY_VIRT_TO_PHYS(kernel_region_top)) {
        panic("Kernel image was loaded in reserved memory.");
    }

    /* TODO Compute sequential allocation limit taking initrd into account */
    /* TODO Report zone limits */
}
