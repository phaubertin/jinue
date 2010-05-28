#include <alloc.h> /* includes kernel.h */
#include <panic.h>
#include <vm.h>

alloc_page_t __alloc_page;


addr_t early_alloc_page(void) {
	addr_t page = kernel_region_top;
	kernel_region_top += PAGE_SIZE;
	
	return page;
}

physaddr_t do_not_call(void) {
	panic("do_not_call()");
	
	return NULL; /* never returns */
}
