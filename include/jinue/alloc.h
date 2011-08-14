#ifndef _JINUE_ALLOC_H_
#define _JINUE_ALLOC_H_

#include <jinue/types.h>

#define KERNEL_PAGE_STACK_SIZE	512

#define KERNEL_PAGE_STACK_INIT	32


typedef struct  {
	physaddr_t addr;
	physsize_t size;
} memory_block_t;

#endif
