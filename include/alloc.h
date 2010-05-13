#ifndef _JINUE_KERNEL_ALLOC_H
#define _JINUE_KERNEL_ALLOC_H

#include <kernel.h>

extern physaddr_t *page_stack;

extern physaddr_t *page_stack_addr;

extern physaddr_t *page_stack_top;

extern unsigned int page_stack_count;


physaddr_t alloc_page(void);

addr_t early_alloc_page(void);

/*
addr_t alloc(size_t size);

void free(addr_t addr);*/

#endif

