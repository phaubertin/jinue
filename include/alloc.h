#ifndef _JINUE_ALLOC_H
#define _JINUE_ALLOC_H

#include <kernel.h>

#define PAGE_SIZE  4096

void alloc_init(void);
addr_t alloc(unsigned int pages);

#endif

