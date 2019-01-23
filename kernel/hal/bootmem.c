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

#include <hal/boot.h>
#include <hal/bootmem.h>
#include <hal/e820.h>
#include <hal/kernel.h>
#include <hal/pfaddr.h>
#include <hal/vm.h>
#include <panic.h>
#include <printk.h>
#include <stddef.h>
#include <types.h>
#include <util.h>


/** kernel memory map */
bootmem_t *ram_map;

/** available memory map (allocator) */
bootmem_t *bootmem_root;

/** current top of boot heap */
void *boot_heap;


void new_ram_map_entry(pfaddr_t addr, uint32_t count, bootmem_t **head) {
    bootmem_t   *entry;
    
    entry     = (bootmem_t *)boot_heap;
    boot_heap = (bootmem_t *)boot_heap + 1;
    
    entry->next  = *head;
    entry->addr  = addr;
    entry->count = count;
    
    *head = entry;
}

void apply_mem_hole(e820_addr_t hole_start, e820_addr_t hole_end, bootmem_t **head) {
    bootmem_t *ptr, **dptr;
    pfaddr_t addr, top;
    pfaddr_t hole_addr, hole_top;
    
    hole_addr = hole_start >> PFADDR_SHIFT;
    hole_top  = hole_end   >> PFADDR_SHIFT;
    
    /* align on page boundaries */
    if( OFFSET_OF(hole_start, PAGE_SIZE) != 0 ) {
        hole_addr = (hole_addr & (e820_addr_t)~(PAGE_MASK >> PFADDR_SHIFT));
    }
    
    if( OFFSET_OF(hole_end, PAGE_SIZE) != 0 ) {
        hole_top = (hole_top & (e820_addr_t)~(PAGE_MASK >> PFADDR_SHIFT)) + (PAGE_SIZE >> PFADDR_SHIFT);
    }
    
    /* apply hole to all available memory blocks */
    for(dptr = head, ptr = *head; ptr != NULL; dptr = &ptr->next, ptr = ptr->next) {
        addr  = ptr->addr;
        top   = addr + ptr->count * (PAGE_SIZE >> PFADDR_SHIFT);
        
        /* case where the block is completely inside the hole */
        if(addr >= hole_addr && top <= hole_top) {
            /* remove this block */
            *dptr = ptr->next;
            
            return;
        }
        
        /* case where the block must be split in two because the hole is
         * inside it */
        if(addr < hole_addr && top > hole_top) {
            /* first block: below the hole */
            ptr->count = (hole_addr - addr) / (PAGE_SIZE >> PFADDR_SHIFT);
            
            /* second block: above the hole */
            new_ram_map_entry(hole_top, (top - hole_top) / (PAGE_SIZE >> PFADDR_SHIFT), head);
            
            return;
        }
        
        /* fix size or addr if block overlaps hole */
        if(addr >= hole_addr && addr < hole_top) {
            ptr->addr = hole_top;
            ptr->count = (top - hole_top) / (PAGE_SIZE >> PFADDR_SHIFT);
            
            return;
        }
        
        if(top > hole_addr && top <= hole_top) {
            ptr->count = (hole_addr - addr) / (PAGE_SIZE >> PFADDR_SHIFT);
        }
    }
}

void bootmem_init(bool use_pae) {
    const addr_t initial_boot_heap = boot_heap;
    
    bootmem_t *ptr;
    bootmem_t *temp_root;
    unsigned int idx;
    
    const boot_info_t *boot_info = get_boot_info();
    
    /** TODO check for available regions overlap */

    /* copy the available ram entries from the e820 map and insert them
     * in a linked list */
    ram_map = NULL;
    
    for(idx = 0; idx < boot_info->e820_entries; ++idx) {
        const e820_t *e820_entry = &boot_info->e820_map[idx];

        if(! e820_is_valid(e820_entry)) {
            continue;
        }

        if( e820_is_available(e820_entry) ) {
            /* get memory entry start and end addresses */
            e820_addr_t start = e820_entry->addr;
            e820_addr_t end   = start + e820_entry->size;
            
            /* align on page boundaries */
            if( OFFSET_OF(start, PAGE_SIZE) != 0 ) {
                start = (start & (e820_addr_t)~PAGE_MASK) + PAGE_SIZE;
            }
            
            if( OFFSET_OF(end, PAGE_SIZE) != 0 ) {
                end = (end & (e820_addr_t)~PAGE_MASK);
            }
            
            /* If Physical Address Extension (PAE) is disabled, memory above the
             * 4GB mark is not usable. */
            if(! use_pae) {
                /* If this memory region is completely above the 4GB mark, exclude it. */
                if(start >= ADDR_4GB) {
                    continue;
                }
                
                /* If this memory region starts below the 4GB mark but extends
                 * beyond it, crop at 4GB. */
                if(end > ADDR_4GB) {
                    end = ADDR_4GB;
                }
            }
            
            /* add entry to linked list */
            if(end > start) {
                new_ram_map_entry(ADDR_TO_PFADDR(start), (uint32_t)(end - start) / PAGE_SIZE, &ram_map);
            }
        }
    }

    /* apply every unavailable entries from the e820 map as holes */
    for(idx = 0; idx < boot_info->e820_entries; ++idx) {
        const e820_t *e820_entry = &boot_info->e820_map[idx];

        if(! e820_is_valid(e820_entry)) {
            continue;
        }

        if( e820_is_available(e820_entry) ) {
            continue;
        }
        
        e820_addr_t start = e820_entry->addr;
        e820_addr_t end   = start + e820_entry->size;
        
        apply_mem_hole(start, end, &ram_map);
    }
    
    /* Apparently, the first 64k of memory are corrupted by some BIOSes. 
         * It would be nice to try to detect this. In the meantime, let's
         * assume the problem is present. */
    apply_mem_hole(0, 0x10000, &ram_map);
    
    /* the kernel image, its heap and stack, and early-allocated pages */
    apply_mem_hole((uint32_t)boot_info->image_start, (uint32_t)kernel_region_top, &ram_map);

    /* Entry removal may have left garbage on the heap (bootmem_t
     * structures which were allocated on the heap but are no longer
     * linked). Let's clean up. */
    temp_root = NULL;
    
    for(ptr = ram_map; ptr != NULL; ptr = ptr->next) {
        new_ram_map_entry(ptr->addr, ptr->count, &temp_root);
    }
    
    ram_map   = NULL;
    boot_heap = initial_boot_heap;
    
    for(ptr = temp_root; ptr != NULL; ptr = ptr->next) {
        new_ram_map_entry(ptr->addr, ptr->count, &ram_map);
    }    
    
    /* at this point, we should have at least one block of available RAM */
    if( ram_map == NULL ) {
        panic("no available memory.");
    }    
    
    /* Let's count and display the total amount of available memory */
    uint32_t page_count = 0;
    for(ptr = ram_map; ptr != NULL; ptr = ptr->next) {
        page_count += ptr->count;
    }
    
    /** TODO this won't work for available memory > 4GB */
    printk("%u kilobytes (%u pages) of memory available.\n", 
        (uint32_t)(page_count * PAGE_SIZE / KB),
        (uint32_t)(page_count) );
    
    /* head pointer for bootmem_get_block() */
    bootmem_root = ram_map;
}

bootmem_t *bootmem_get_block(void) {
    bootmem_t *block;
    
    block = bootmem_root;
    
    if(block != NULL) {
        bootmem_root = block->next;
    }
    
    return block;
}
