#ifndef _JINUE_ALLOC_H
#define _JINUE_ALLOC_H

#include <kernel.h>

#define PAGE_BITS 12
#define PAGE_SIZE (1<<PAGE_BITS) /* 2**12 = 4096 */

void alloc_init(void);
addr_t alloc(size_t size);

#endif

