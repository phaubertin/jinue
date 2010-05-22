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

