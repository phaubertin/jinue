#include <hal/vm.h>
#include <hal/kernel.h>
#include <assert.h>
#include <panic.h>
#include <pfalloc.h>
#include <stddef.h>


bool use_pfalloc_early;

pfcache_t global_pfcache;


addr_t pfalloc_early(void) {
	addr_t page;
	
	/** ASSERTION:  pfalloc_early is used early only */
	assert(use_pfalloc_early);
	
	page = kernel_region_top;
	kernel_region_top += PAGE_SIZE;
	
	return page;
}

void init_pfcache(pfcache_t *pfcache, pfaddr_t *stack_page) {
	pfaddr_t *ptr;
	unsigned int idx;
	
	ptr = stack_page;
	
	for(idx = 0;idx < KERNEL_PAGE_STACK_SIZE; ++idx) {
		ptr[idx] = PFNULL;
	}
	
	pfcache->ptr   = stack_page;
	pfcache->count = 0;
}

pfaddr_t pfalloc_from(pfcache_t *pfcache) {
	/** ASSERTION:  pfalloc_early must be used early */
	assert( ! use_pfalloc_early );
	
	if(pfcache->count == 0) {
		panic("pfalloc_from(): no more pages to allocate");
	}	
	
	--pfcache->count;
	
	return *(--pfcache->ptr);
}

void pffree_to(pfcache_t *pfcache, pfaddr_t pf) {
	if(pfcache->count >= KERNEL_PAGE_STACK_SIZE) {
		/** We are leaking memory here. Should we panic instead? */
		return;
	}
	
	++pfcache->count;
	
	(pfcache->ptr++)[0] = pf;
}
