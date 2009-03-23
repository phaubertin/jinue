#ifndef _JINUE_KERNEL_ALLOC_H
#define _JINUE_KERNEL_ALLOC_H

#include <kernel.h>

void alloc_init(void);

addr_t alloc(size_t size);

void free(addr_t addr);

#endif

