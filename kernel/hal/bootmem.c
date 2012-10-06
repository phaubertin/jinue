#include <bootmem.h>
#include <e820.h>
#include <kbd.h>
#include <kernel.h>
#include <panic.h>
#include <printk.h>
#include <stddef.h>
#include <util.h>
#include <vm.h>


/** kernel memory map */
bootmem_t *ram_map;

/** available memory map (allocator) */
bootmem_t *bootmem_root;

/** current top of boot heap */
addr_t boot_heap;


void new_ram_map_entry(pfaddr_t addr, uint32_t count, bootmem_t **head) {
	( (bootmem_t *)boot_heap )->next = *head;
	*head = (bootmem_t *)boot_heap;
	boot_heap = (bootmem_t *)boot_heap + 1;
	
	(*head)->addr  = addr;
	(*head)->count = count;
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
	
	/* apply_mem_hole(prev, ADDR_TO_PFADDR( e820_get_addr(idx) ), e820_get_size(idx) / PAGE_SIZE, &ram_map); */
}

void bootmem_init(void) {
	const addr_t initial_boot_heap = boot_heap;
	
	bootmem_t *ptr;
	bootmem_t *temp_root;
	e820_addr_t start, end;
	uint32_t   size;
	unsigned int idx;
	
	/* copy the available ram entries from the e820 map and insert them
	 * in a linked list */
	ram_map = NULL;
	
	for(idx = 0; e820_is_valid(idx); ++idx) {
		if( e820_is_available(idx) ) {
			/* get memory entry start and end addresses */
			start = e820_get_addr(idx);
			end   = start + e820_get_size(idx);
			
			/* align on page boundaries */
			if( OFFSET_OF(start, PAGE_SIZE) != 0 ) {
				start = (start & (e820_addr_t)~PAGE_MASK) + PAGE_SIZE;
			}
			
			if( OFFSET_OF(end, PAGE_SIZE) != 0 ) {
				end = (end & (e820_addr_t)~PAGE_MASK);
			}
			
			/* add entry to linked list */
			if(end > start) {
				new_ram_map_entry(ADDR_TO_PFADDR(start), (uint32_t)(end - start) / PAGE_SIZE, &ram_map);
			}
		}
	}

	/* apply every unavailable entries from the e820 map as holes */
	for(idx = 0; e820_is_valid(idx); ++idx) {
		if( e820_is_available(idx) ) {
			continue;
		}
		
		start = e820_get_addr(idx);
		end   = start + e820_get_size(idx);
		
		apply_mem_hole(start, end, &ram_map);
	}
	
	/* Apparently, the first 64k of memory are corrupted by some BIOSes. 
		 * It would be nice to try to detect this. In the meantime, let's
		 * assume the problem is present. */
	apply_mem_hole(0, 0x10000, &ram_map);
	
	/* the kernel image and its heap and stack early-allocated pages */
	apply_mem_hole((uint32_t)kernel_start, (uint32_t)kernel_region_top, &ram_map);

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
	size = 0;
	for(ptr = ram_map; ptr != NULL; ptr = ptr->next) {
		size += ptr->count;
	}
	
	printk("%u kilobytes (%u pages) of memory available.\n", 
		(uint32_t)(size * PAGE_SIZE / 1024), 
		(uint32_t)(size) );
	
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