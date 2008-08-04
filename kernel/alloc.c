#include <bios.h>
#include <kernel.h>

#define MEM_BOTTOM 0x200000
#define PAGE_SIZE  4096

static addr_t _alloc_addr;
static unsigned int _alloc_size;

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
	_alloc_size = best_size / PAGE_SIZE;
	
	printk("%u kilobytes (%u pages) available starting at %xh.\n", 
		_alloc_size * PAGE_SIZE / 1024, 
		_alloc_size, 
		_alloc_addr );
}

addr_t alloc(unsigned int pages) {
	addr_t addr;
	
	if(_alloc_size < pages) {
		panic("out of memory.");
	}
	
	addr = _alloc_addr;
	_alloc_addr += pages * PAGE_SIZE;
	_alloc_size -= pages;
	
	return addr;	
}

