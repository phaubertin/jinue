#ifndef _JINUE_COMMON_PFALLOC_H
#define _JINUE_COMMON_PFALLOC_H

#include <jinue-common/pfaddr.h>

#define KERNEL_PAGE_STACK_SIZE    1024

#define KERNEL_PAGE_STACK_INIT    128


typedef struct  {
    pfaddr_t addr;
    uint32_t count;
} memory_block_t;

#endif
