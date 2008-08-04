#include <bios.h>
#include <kernel.h>
#include <stddef.h>

#define MEM_BOTTOM 0x200000

static addr_t _alloc_addr;
static size_t _alloc_size;

void alloc_init(void) {
	unsigned int idx;	
	bool avail;
	unsigned int addr, size, type;
	unsigned int best_addr, best_size;
	unsigned int fixed_addr, fixed_size;
	
	idx = 0;
	best_size = 0;
	
	printk("Dump of the BIOS memory map:\n");
	printk("  address  size     type\n");
	while( e820_is_valid(idx) ) {
		addr = (unsigned int)e820_get_addr(idx);
		size = (unsigned int)e820_get_size(idx);
		type = (unsigned int)e820_get_type(idx);
		avail = e820_is_available(idx);
		
		++idx;
		
		printk("%c %x %x %s\n",
			avail?'*':' ',
			addr,
			size,
			e820_type_description(type) );
		
		if( avail ) {
			if(addr + size < MEM_BOTTOM) {
				continue;
			}
			
			fixed_addr = addr;
			fixed_size = size;
			if(fixed_addr < MEM_BOTTOM) {
				fixed_addr = MEM_BOTTOM;
				fixed_size -= fixed_addr - addr;								
			}
			
			if(fixed_size > best_size) {
				best_addr = fixed_addr;
				best_size = fixed_size;
			}
		}
	}
	
	if( best_size == 0 ) {
		panic("no memory to allocate.");
	}
	
	_alloc_addr = (addr_t)best_addr;
	_alloc_size = (size_t)best_size;
	
	best_size /= 1024;
		
	printk("%u kilobytes available to allocate starting at %xh.\n", best_size, best_addr);
}

