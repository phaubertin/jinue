/*
 * Copyright (C) 2025-2026 Philippe Aubertin.
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

#include <jinue/shared/asm/mman.h>
#include <kernel/domain/services/mman.h>
#include <kernel/domain/services/panic.h>
#include <kernel/machine/asm/machine.h>
#include <kernel/machine/pmap.h>
#include <kernel/machine/spinlock.h>
#include <kernel/utils/utils.h>
#include <stdbool.h>

typedef struct {
    addr_t       addr;
    size_t       size_remaining;
    size_t       page_size;
} alloc_region_t;

alloc_region_t normal_region = {
    .addr           = (addr_t)MAPPING_AREA_ADDR,
    .size_remaining = MAPPING_AREA_SIZE,
    .page_size      = PAGE_SIZE,
};

alloc_region_t large_pages_region = {
    .addr           = (addr_t)LARGE_PAGES_AREA_ADDR,
    .size_remaining = LARGE_PAGES_AREA_SIZE,
    /* set by map_in_kernel() */
    .page_size      = 0,
};

static struct {
    alloc_region_t  *region;
    addr_t           addr;
    const void      *latest_addr;
    int              latest_prot;
    int              latest_flags;
    size_t           size_remaining;
} alloc_state = {
    .region         = NULL,
    .latest_addr    = NULL,
    .latest_prot    = JINUE_PROT_NONE,
    .latest_flags   = JINUE_MAP_NONE,
};

/**
 * Map new pages to expand the last mapping in the mapping area.
 * 
 * @param paddr physical address of start, must be page-aligned
 * @param new_end new end of the expanded mapping, must be page aligned
 * @param prot protection flags
 * @param flags mapping flags
 */
static void expand_mapping(paddr_t paddr, addr_t new_end, int prot, int flags) {
    alloc_region_t *region = alloc_state.region;

    addr_t old_end  = region->addr;
    size_t size     = new_end - old_end;

    if(size > region->size_remaining) {
        panic("No more space to map memory in kernel");
    }

    machine_map_kernel(old_end, size, paddr, prot, flags);

    region->addr            = new_end;
    region->size_remaining  -= size;
}

/**
 * Unmap pages to shrink the last mapping in the mapping area.
 * 
 * @param new_end new end of the shrunk mapping, must be page aligned
 */
static void shrink_mapping(addr_t new_end) {
    alloc_region_t *region = alloc_state.region;

    addr_t old_end  = region->addr;
    size_t size     = old_end - new_end;

    machine_unmap_kernel(new_end, size);

    region->addr            = new_end;
    region->size_remaining  += size;
}

/**
 * Permanently map memory in the kernel's mapping area
 * 
 * Sufficient virtual memory is allocated in the mapping area, which ranges
 * from MAPPING_AREA_ADDR to MAPPING_AREA_END. This function panics if
 * sufficient virtual memory cannot be allocated in this range.
 * 
 * There are no alignment requirements: this function takes care of aligning
 * the mapping on page boundaries.
 * 
 * This function is not thread safe and is intended to be called only during
 * kernel initialization.
 * 
 * @param paddr address to memory map
 * @param size size of memory to map, cannot be zero
 * @param prot protection flags
 * @param flags mapping flags
 */
void *map_in_kernel(paddr_t paddr, size_t size, int prot, int flags) {
    alloc_region_t *region;

    if(flags & JINUE_MAP_LARGE_PAGES) {
        large_pages_region.page_size = machine_large_page_size();
        region = &large_pages_region;
    }
    else {
        region = &normal_region;
    }

    size_t offset   = paddr & (region->page_size - 1);

    addr_t start    = region->addr;
    addr_t end      = ALIGN_END_PTR(start + offset + size, region->page_size);
    
    alloc_state.region          = region;
    alloc_state.latest_addr     = start + offset;
    alloc_state.latest_prot     = prot;
    alloc_state.latest_flags    = flags;

    expand_mapping(paddr - offset, end, prot, flags);

    return start + offset;
}

/**
 * Resize mapping established by the latest call to map_in_kernel()
 * 
 * @param size size of memory to map, cannot be zero
 */
void resize_map_in_kernel(size_t size) {
    alloc_region_t *region = alloc_state.region;

    const void *addr    = alloc_state.latest_addr;

    addr_t old_end      = alloc_state.region->addr;
    addr_t new_end      = ALIGN_END_PTR((addr_t)addr + size, region->page_size);

    alloc_state.region->addr    = new_end;

    if(new_end <= old_end) {
        shrink_mapping(new_end);
    } else {
        int prot        = alloc_state.latest_prot;
        int flags       = alloc_state.latest_flags;

        addr_t start    = ALIGN_START_PTR(addr, region->page_size);
        paddr_t paddr   = machine_lookup_kernel_paddr(start) + (old_end - start);

        expand_mapping(paddr, new_end, prot, flags);
    }
}

/**
 * Undo (unmap) the mapping established by the latest call to map_in_kernel()
 * 
 * @param addr address returned by map_in_kernel() for the mapping being undone
 */
void undo_map_in_kernel(void) {
    alloc_region_t *region = alloc_state.region;

    const void *addr = alloc_state.latest_addr;

    void *start = ALIGN_START_PTR(addr, region->page_size);

    shrink_mapping(start);
    
    alloc_state.latest_addr = NULL;
    alloc_state.latest_prot = JINUE_PROT_NONE;
    alloc_state.latest_flags = JINUE_MAP_NONE;
}
