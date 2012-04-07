#ifndef _JINUE_KERNEL_ALLOC_H
#define _JINUE_KERNEL_ALLOC_H

#include <jinue/alloc.h>
#include <jinue/types.h>
#include <stdbool.h>
#include <stdint.h>


#define alloc_page() ( (*__alloc_page)() )

#define free_page(paddr)


typedef pfaddr_t (*alloc_page_t)(void);

typedef struct {
	pfaddr_t  *ptr;
	uint32_t   count;
} page_stack_t;


extern alloc_page_t __alloc_page;

extern bool use_alloc_page_early;

extern page_stack_t *page_stack;

extern page_stack_t __page_stack;


addr_t alloc_page_early(void);

pfaddr_t do_not_call(void);

void init_page_stack(page_stack_t *stack, pfaddr_t *stack_addr);

pfaddr_t stack_alloc_page(void);

void stack_free_page(pfaddr_t page);

#endif
