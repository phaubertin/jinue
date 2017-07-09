#ifndef _JINUE_ALLOC_H
#define _JINUE_ALLOC_H

#include <jinue/types.h>

#define KERNEL_PAGE_STACK_SIZE    1024

#define KERNEL_PAGE_STACK_INIT    128


typedef struct  {
    pfaddr_t addr;
    uint32_t count;
} memory_block_t;

#endif
