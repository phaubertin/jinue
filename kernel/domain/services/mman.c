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

#include <jinue/shared/asm/mman.h>
#include <kernel/domain/services/mman.h>
#include <kernel/domain/services/panic.h>
#include <kernel/machine/asm/machine.h>
#include <kernel/machine/pmap.h>
#include <kernel/machine/spinlock.h>
#include <kernel/utils/utils.h>
#include <stdbool.h>

static struct {
    addr_t       addr;
    const void  *latest_addr;
    int          latest_prot;
    size_t       size_remaining;
} alloc_state = {
    .addr           = (addr_t)MAPPING_AREA_ADDR,
    .latest_addr    = NULL,
    .latest_prot    = JINUE_PROT_NONE,
    .size_remaining = MAPPING_AREA_SIZE,
};

/**
 * Map new pages to expand the last mapping in the mapping area.
 * 
 * @param paddr physical address of start, must be page-aligned
 * @param new_end new end of the expanded mapping, must be page aligned
 * @param prot mapping protection flags
 */
static void expand_mapping(paddr_t paddr, addr_t new_end, int prot) {
    addr_t old_end  = alloc_state.addr;
    size_t size     = new_end - old_end;

    if(size > alloc_state.size_remaining) {
        panic("No more space to map memory in kernel");
    }

    /* Be careful about the possible address overflow here since the mapping
     * area is at the very top of the address space. This is why we are using
     * a condition on the offset rather than the address. */
    for(int offset = 0; offset < size; offset += PAGE_SIZE) {
        machine_map_kernel_page(old_end + offset, paddr + offset, prot);
    }

    alloc_state.addr            = new_end;
    alloc_state.size_remaining  -= size;
}

/**
 * Unmap pages to shrink the last mapping in the mapping area.
 * 
 * @param new_end new end of the shrunk mapping, must be page aligned
 */
static void shrink_mapping(addr_t new_end) {
    addr_t old_end  = alloc_state.addr;
    size_t size     = old_end - new_end;

    /* Be careful about the possible address overflow here since the mapping
     * area is at the very top of the address space. This is why we are using
     * a condition on the offset rather than the address. */
    for(int offset = 0; offset < size; offset += PAGE_SIZE) {
        machine_unmap_kernel_page(new_end + offset);
    }
    
    alloc_state.addr            = new_end;
    alloc_state.size_remaining  += size;
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
 * @param prot mapping protection flags
 */
void *map_in_kernel(paddr_t paddr, size_t size, int prot) {
    size_t offset   = paddr % PAGE_SIZE;

    addr_t start    = alloc_state.addr;
    addr_t end      = ALIGN_END_PTR(start + offset + size, PAGE_SIZE);
    
    alloc_state.latest_addr = start + offset;
    alloc_state.latest_prot = prot;

    expand_mapping(paddr - offset, end, prot);

    return start + offset;
}

/**
 * Resize mapping established by the latest call to map_in_kernel()
 * 
 * @param size size of memory to map, cannot be zero
 */
void resize_map_in_kernel(size_t size) {
    const void *addr    = alloc_state.latest_addr;

    addr_t old_end      = alloc_state.addr;
    addr_t new_end      = ALIGN_END_PTR((addr_t)addr + size, PAGE_SIZE);

    alloc_state.addr    = new_end;

    if(new_end <= old_end) {
        shrink_mapping(new_end);
    } else {
        int prot        = alloc_state.latest_prot;

        void *start     = ALIGN_START_PTR(addr, PAGE_SIZE);
        paddr_t paddr   = machine_lookup_kernel_paddr(start) + (new_end - old_end);

        expand_mapping(paddr, new_end, prot);
    }
}

/**
 * Undo (unmap) the mapping established by the latest call to map_in_kernel()
 * 
 * @param addr address returned by map_in_kernel() for the mapping being undone
 */
void undo_map_in_kernel(void) {
    const void *addr = alloc_state.latest_addr;

    void *start = ALIGN_START_PTR(addr, PAGE_SIZE);

    shrink_mapping(start);
    
    alloc_state.latest_addr = NULL;
    alloc_state.latest_prot = JINUE_PROT_NONE;
}
