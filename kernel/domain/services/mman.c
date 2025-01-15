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

#include <kernel/domain/services/mman.h>
#include <kernel/domain/services/panic.h>
#include <kernel/machine/asm/machine.h>
#include <kernel/machine/pmap.h>
#include <kernel/machine/spinlock.h>
#include <kernel/utils/utils.h>

static struct {
    addr_t      addr;
    spinlock_t  lock;
} alloc_state = {
    .addr   = (addr_t)MAPPING_AREA_ADDR,
    .lock   = SPINLOCK_INITIALIZER
};

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
 * @param paddr address to memory map
 * @param size size of memory to map
 * @param prot mapping protection flags
 */
void *map_in_kernel(kern_paddr_t paddr, size_t size, int prot) {
    size_t offset = paddr % PAGE_SIZE;
    
    size += offset;

    spin_lock(&alloc_state.lock);

    if((addr_t)MAPPING_AREA_END - alloc_state.addr < size) {
        spin_unlock(&alloc_state.lock);
        panic("No more space to map memory in kernel");
    }

    addr_t start = alloc_state.addr;
    alloc_state.addr = ALIGN_END_PTR(start + size, PAGE_SIZE);

    spin_unlock(&alloc_state.lock);

    addr_t map_addr = start;
    kern_paddr_t map_paddr = paddr - offset;

    while(true) {
        machine_map_kernel_page(map_addr, map_paddr, prot);

        if(size <= PAGE_SIZE) {
            break;
        }

        map_addr += PAGE_SIZE;
        map_paddr += PAGE_SIZE;
        size -= PAGE_SIZE;
    }

    return start + offset;
}

/**
 * Expand a mapping established by map_in_kernel()
 * 
 * This function is intended for use during boot and has the following caveats:
 * - It assumes the mapping being expanded is the last mapping performed.
 * - This function does not perform locking. It is intended for use by the
 *   first running CPU only during boot.
 * 
 * @param addr address returned by map_in_kernel() for the mapping being expanded
 * @param size size of memory to map
 * @param prot mapping protection flags
 * 
*/
void expand_map_in_kernel(const void *addr, size_t size, int prot) {
    void *start     = ALIGN_START_PTR(addr, PAGE_SIZE);
    addr_t old_end  = alloc_state.addr;
    addr_t new_end  = ALIGN_END_PTR((addr_t)addr + size, PAGE_SIZE);

    kern_paddr_t paddr = machine_lookup_kernel_paddr(start) + (new_end - old_end);

    for(addr_t page_addr = old_end; page_addr < (addr_t)new_end; page_addr += PAGE_SIZE) {
        machine_map_kernel_page(page_addr, paddr, prot);
        ++paddr;
    }

    alloc_state.addr = new_end;
}

/**
 * Undo a mapping established by map_in_kernel()
 * 
 * This function is intended for use during boot and has the following caveats:
 * - *All* mappings done since the one identified by the argument are undone.
 *   Typical usage of this function is to undo the last mapping performed.
 * - This function does not perform locking. It is intended for use by the
 *   first running CPU only during boot.
 * 
 * @param addr address returned by map_in_kernel() for the mapping being undone
 * 
*/
void undo_map_in_kernel(void *addr) {
    void *start = ALIGN_START_PTR(addr, PAGE_SIZE);
    void *end   = alloc_state.addr;

    for(addr_t page_addr = start; page_addr < (addr_t)end; page_addr += PAGE_SIZE) {
        machine_unmap_kernel_page(page_addr);
    }

    alloc_state.addr = start;
}
