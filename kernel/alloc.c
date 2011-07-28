#include <alloc.h> /* includes kernel.h */
#include <assert.h>
#include <panic.h>
#include <vm.h>

alloc_page_t __alloc_page;

bool use_alloc_page_early;


addr_t alloc_page_early(void) {
	/** ASSERTION:  alloc_page_early is used early only */
	assert(use_alloc_page_early);
	
	addr_t page = kernel_region_top;
	kernel_region_top += PAGE_SIZE;
	
	return page;
}

physaddr_t do_not_call(void) {
	panic("do_not_call()");
	
	return NULL; /* never returns */
}
