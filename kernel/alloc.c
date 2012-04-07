#include <alloc.h>
#include <assert.h>
#include <panic.h>
#include <stddef.h>
#include <vm.h>

alloc_page_t __alloc_page;

bool use_alloc_page_early;

page_stack_t __page_stack;

page_stack_t *page_stack;


addr_t alloc_page_early(void) {
	/** ASSERTION:  alloc_page_early is used early only */
	assert(use_alloc_page_early);
	
	addr_t page = kernel_region_top;
	kernel_region_top += PAGE_SIZE;
	
	return page;
}

pfaddr_t do_not_call(void) {
	panic("do_not_call()");
	
	return NULL; /* never returns */
}


void init_page_stack(page_stack_t *stack, pfaddr_t *stack_addr) {
	pfaddr_t *ptr;
	unsigned int idx;
	
	ptr = stack_addr;
	
	for(idx = 0;idx < KERNEL_PAGE_STACK_SIZE; ++idx) {
		ptr[idx] = (pfaddr_t)NULL;
	}
	
	stack->ptr   = stack_addr;
	stack->count = 0;
}

pfaddr_t stack_alloc_page(void) {
	if(page_stack->count == 0) {
		panic("stack_alloc_page(): no more pages to allocate");
	}	
	
	--page_stack->count;
	
	return *(--page_stack->ptr);
}

void stack_free_page(pfaddr_t page) {
	if(page_stack->count >= KERNEL_PAGE_STACK_SIZE) {
		/** We are leaking memory here. Should we panic instead? */
		return;
	}
	
	++page_stack->count;
	
	(page_stack->ptr++)[0] = page;
}
