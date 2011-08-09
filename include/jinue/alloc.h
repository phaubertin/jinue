#ifndef _JINUE_ALLOC_H_
#define _JINUE_ALLOC_H_

#define KERNEL_PAGE_LIST_SIZE	1024

#define KERNEL_PAGE_LIST_INIT	16

#define MAX_MEM_BLOCK_COUNT		64


struct memblock_t {
	physaddr_t addr;
	physsize_t size;
};

#endif
