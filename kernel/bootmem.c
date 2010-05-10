#include <boot.h>
#include <bootmem.h>
#include <kernel.h>
#include <panic.h>
#include <stdbool.h>
#include <vm.h>

bootmem_t *ram_map;
bootmem_t *bootmem_root;
addr_t boot_heap;

bootmem_t *new_ram_map_entry(physaddr_t addr, physsize_t size) {
	( (bootmem_t *)boot_heap )->next = ram_map;
	ram_map = (bootmem_t *)boot_heap;
	boot_heap += sizeof(bootmem_t);
	
	ram_map->addr = addr;
	ram_map->size = size;
	
	return ram_map;
}			

void bootmem_init(void) {
	const physaddr_t hole_start = 0xA0000;
	const physaddr_t hole_top   = (physaddr_t)(unsigned int)kernel_region_top;	
	
	physaddr_t addr;
	physsize_t size;
	physsize_t remainder;
	e820_type_t type;
	bool avail;
	unsigned int idx;
	
	idx = 0;
	ram_map = NULL;
	
	/* process the bios memory map to create a map of available memory */
	while( e820_is_valid(idx) ) {
		addr = e820_get_addr(idx);
		size = e820_get_size(idx);
		type = e820_get_type(idx);
		avail = e820_is_available(idx);
		
		++idx;
				
		if( !avail ) {
			continue;
		}
		
		/* align left block boundary on page boundary */
		remainder = addr % PAGE_SIZE;
		if(remainder != 0) {
			remainder = PAGE_SIZE - remainder;
			if(size < remainder) {
				continue;
			}
			
			addr += remainder;
			size -= remainder;
		}
		
		/* align right block boundary on page boundary */
		remainder = size % PAGE_SIZE;
		if(remainder != 0) {
			if(size < remainder) {
				continue;
			}
			
			size -= remainder;
		}
		
		/* case where the block is completely inside the hole */
		if(addr >= hole_start && addr + size <= hole_top) {
			continue;
		}
		
		/* case where the block must be split in two because the hole is
		 * inside it */
		if(addr < hole_start && addr + size > hole_top) {
			/* first block: below the hole */
			(void)new_ram_map_entry(addr, hole_start - addr);
			
			/* second block: above the hole */
			(void)new_ram_map_entry(hole_top, addr + size - hole_top);
		}
		
		
		/* fix size or addr if block overlaps hole */
		if(addr >= hole_start && addr < hole_top) {
			size -= hole_top - addr;
			addr = hole_top;
		}
		
		if(addr + size > hole_start && addr + size <= hole_top) {
			size = addr - hole_start;
		}
		
		/* new block */
		(void)new_ram_map_entry(addr, size);			
	}
	
	/* at this point, we should have at least one block of available RAM */
	if( ram_map == NULL ) {
		panic("no available memory.");
	}	
}
