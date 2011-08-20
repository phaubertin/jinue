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

physaddr_t do_not_call(void) {
	panic("do_not_call()");
	
	return NULL; /* never returns */
}


void init_page_stack(page_stack_t *stack, physaddr_t *stack_addr) {
	physaddr_t *ptr;
	physaddr_t new_page;
	unsigned int idx;
	
	ptr = stack_addr;
	
	for(idx = 0; idx < KERNEL_PAGE_STACK_INIT; ++idx) {		
		new_page = alloc_page();
		
		if(new_page == (physaddr_t)NULL) {
			break;
		}
			
		*(ptr++) = new_page;
	}
	
	stack->ptr   = ptr;
	stack->count = idx;
	
	for(;idx < KERNEL_PAGE_STACK_SIZE; ++idx) {
		*(ptr++) = (physaddr_t)NULL;
	}	
}

physaddr_t stack_alloc_page(void) {
	if(page_stack->count == 0) {
		return (physaddr_t)NULL;
	}	
	
	--page_stack->count;
	
	return *(--page_stack->ptr);
}

void stack_free_page(physaddr_t page) {
	if(page_stack->count >= KERNEL_PAGE_STACK_SIZE) {
		/** should we panic here ? */
		return;
	}
	
	(page_stack->ptr++)[0] = page;
}
