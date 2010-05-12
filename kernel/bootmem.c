#include <boot.h>
#include <bootmem.h>
#include <kernel.h>
#include <panic.h>
#include <printk.h>
#include <stdbool.h>
#include <vm.h>

bootmem_t *ram_map;

bootmem_t *bootmem_root;

addr_t boot_heap;

void new_ram_map_entry(physaddr_t addr, physsize_t size, bootmem_t **head) {
	( (bootmem_t *)boot_heap )->next = *head;
	*head = (bootmem_t *)boot_heap;
	boot_heap += sizeof(bootmem_t);
	
	(*head)->addr = addr;
	(*head)->size = size;
}

void apply_mem_hole(bootmem_t **ptr, physaddr_t hole_addr, physsize_t hole_size, bootmem_t **head) {	
	physaddr_t addr, top;
	physaddr_t hole_top;
	physsize_t size;
	
	addr     = (*ptr)->addr;
	size     = (*ptr)->size;
	top      = addr + size;
	hole_top = hole_addr + hole_size;
	
	
	/* case where the block is completely inside the hole */
	if(addr >= hole_addr && top <= hole_top) {
		/* remove this block */
		*ptr = (*ptr)->next;
		
		return;
	}
	
	/* case where the block must be split in two because the hole is
	 * inside it */
	if(addr < hole_addr && top > hole_top) {
		/* first block: below the hole */
		(*ptr)->size = hole_addr - addr;
		
		/* second block: above the hole */
		new_ram_map_entry(hole_top, top - hole_top, head);
		
		return;
	}
	
	/* fix size or addr if block overlaps hole */
	if(addr >= hole_addr && addr < hole_top) {
		(*ptr)->addr = hole_top;
		(*ptr)->size = top - hole_top;
		
		return;
	}
	
	if(top > hole_addr && top <= hole_top) {
		(*ptr)->size = hole_addr - addr;
	}
}

void bootmem_init(void) {
	const addr_t initial_boot_heap = boot_heap;
	
	bootmem_t *ptr, **prev;	
	bootmem_t *temp_root;
	physsize_t remainder, size;
	unsigned int idx;

	ram_map = NULL;
	
	/* copy the available ram entries from the e820 map and insert them
	 * in a linked list */
	for(idx = 0; e820_is_valid(idx); ++idx) {
		if( e820_is_available(idx) ) {
			new_ram_map_entry(e820_get_addr(idx), e820_get_size(idx), &ram_map);
		}
	}
	
	/* apply every unavailable entries from the e820 map as holes */
	for(idx = 0; e820_is_valid(idx); ++idx) {
		if( e820_is_available(idx) ) {
			continue;
		}
		
		for(ptr = ram_map, prev = &ram_map; ptr != NULL; prev = &ptr->next, ptr = ptr->next) {
			apply_mem_hole(prev, e820_get_addr(idx), e820_get_size(idx), &ram_map);
		}
	}
	
	
	/* other, well known, holes */
	for(ptr = ram_map, prev = &ram_map; ptr != NULL; prev = &ptr->next, ptr = ptr->next) {
		/* the kernel image and its heap and stack */
		apply_mem_hole_range(
			prev,
			(physaddr_t)(unsigned int)kernel_start,
			(physaddr_t)(unsigned int)kernel_region_top,
			&ram_map );
		
		/* Apparently, the first 64k of memory are corrupted by some BIOSes. 
		 * It would be nice to try to detect this. In the meantime, let's
		 * assume the problem is present. */
		apply_mem_hole_range(prev, 0, 0x10000, &ram_map);
	}
		
	/* align blocks on page boundaries */
	for(ptr = ram_map, prev = &ram_map; ptr != NULL; prev = &ptr->next, ptr = ptr->next) {
		/* if size of block is less than one page, remove it */
		if(ptr->size < PAGE_SIZE) {
			(*prev) = ptr->next;			
			continue;
		}
		
		/* left boundary */
		remainder = ptr->addr % PAGE_SIZE;
		if(remainder != 0) {
			remainder = PAGE_SIZE - remainder;			
			ptr->addr += remainder;
			ptr->size -= remainder;
			
			/* if block is now smaller than one page, it will have to
			 * be removed when aligning to right boundary */
			if(ptr->size < PAGE_SIZE) {
				(*prev) = ptr->next;
				continue;
			}
		}
		
		/* right boundary */		
		ptr->size -= ( ptr->size % PAGE_SIZE );
	}
	
	/* Entry removal may have left garbage on the heap (bootmem_t
	 * structures which were allocated on the heap but are no longer
	 * linked). Let's clean up. */
	temp_root = NULL;
	
	for(ptr = ram_map; ptr != NULL; ptr = ptr->next) {
		new_ram_map_entry(ptr->addr, ptr->size, &temp_root);
	}
	
	ram_map   = NULL;
	boot_heap = initial_boot_heap;
	
	for(ptr = temp_root; ptr != NULL; ptr = ptr->next) {
		new_ram_map_entry(ptr->addr, ptr->size, &ram_map);
	}	
	
	/* at this point, we should have at least one block of available RAM */
	if( ram_map == NULL ) {
		panic("no available memory.");
	}	
	
	/* Let's count and display the total amount of available memory */
	size = 0;
	for(ptr = ram_map; ptr != NULL; ptr = ptr->next) {
		size += ptr->size;
	}
	
	printk("%u kilobytes (%u pages) of memory available.\n", 
		(unsigned long)(size / 1024), 
		(unsigned long)(size / PAGE_SIZE) );
	
	/* make a copy of the available memory map for the allocator */
	bootmem_root = NULL;
	
	for(ptr = ram_map; ptr != NULL; ptr = ptr->next) {
		new_ram_map_entry(ptr->addr, ptr->size, &bootmem_root);
	}
}
