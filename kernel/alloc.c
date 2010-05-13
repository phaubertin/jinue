#include <alloc.h> /* includes kernel.h */
#include <vm.h>

/*#include <assert.h>
#include <boot.h>
#include <panic.h>
#include <printk.h>
#include <stddef.h>
*/

physaddr_t *page_stack;

physaddr_t *page_stack_addr;

physaddr_t *page_stack_top;

unsigned int page_stack_count;

physaddr_t alloc_page(void) {
	if(page_stack > page_stack_top) {
		--page_stack_count;
		return *(page_stack++);
	}
	
	return NULL;
}


addr_t early_alloc_page(void) {
	addr_t page = kernel_region_top;
	kernel_region_top += PAGE_SIZE;
	
	return page;
}

#if 0
static addr_t _alloc_addr;
static unsigned int _alloc_size;


void alloc_init(void) {
	unsigned int idx;
	unsigned int remainder;
	bool avail;
	size_t size;
	e820_type_t type;
	addr_t addr, fixed_addr, best_addr;
	size_t fixed_size, best_size;
	
	idx = 0;
	best_size = 0;
	
	while( e820_is_valid(idx) ) {
		addr = (addr_t)e820_get_addr(idx);
		size = (size_t)e820_get_size(idx);
		type = e820_get_type(idx);
		avail = e820_is_available(idx);
		
		++idx;
		
		if( !avail ) {
			continue;
		}
		
		fixed_addr = addr;
		fixed_size = size;
		
		/* is the region completely under the kernel ? */
		if(addr + size > kernel_start) {
			/* is the region completely above the kernel ? */
			if(addr < kernel_region_top) {			
				/* if the region touches the kernel, we take only
				 * the part above the kernel, if there is one... */
				if(addr + size <= kernel_region_top) {
					/* ... and apparently, there is none */
					continue;
				}
				
				fixed_addr = kernel_region_top;
				fixed_size -= fixed_addr - addr;				
			}
		}
		
		/* we must make sure the starting address is aligned on a
		 * page boundary. The size will eventually be divided 
		 * by the page size, and thus need not be aligned. */
		remainder = (unsigned int)fixed_addr % PAGE_SIZE;
		if(remainder != 0) {
			remainder = PAGE_SIZE - remainder;
			if(fixed_size < remainder) {
				continue;
			}
			
			fixed_addr += remainder;
			fixed_size -= remainder;
		}
		 		
		if(fixed_size > best_size) {
			best_addr = fixed_addr;
			best_size = fixed_size;
		}
	}
	
	_alloc_addr = (addr_t)best_addr;
	_alloc_size = best_size / PAGE_SIZE;
	
	if(_alloc_size == 0) {
		panic("no memory to allocate.");
	}
	
	printk("%u kilobytes (%u pages) available starting at %xh.\n", 
		_alloc_size * PAGE_SIZE / 1024, 
		_alloc_size, 
		_alloc_addr );
}

addr_t alloc(size_t size) {
	addr_t addr;
	size_t pages;
	
	pages = size >> PAGE_BITS;
	
	if( (size & PAGE_MASK) != 0 ) {
		++pages;
	}
	
	if(_alloc_size < pages) {
		panic("out of memory.");
	}
	
	addr = _alloc_addr;
	_alloc_addr += pages * PAGE_SIZE;
	_alloc_size -= pages;
	
	/** ASSERTION: returned address should be aligned with a page boundary */
	assert( ((unsigned long)addr & PAGE_MASK) == 0 );
	
	return addr;
}

void free(addr_t addr) {
	/** ASSERTION: we assume starting address is aligned on a page boundary */
	assert( PAGE_OFFSET_OF(addr) == 0 );
}
#endif

