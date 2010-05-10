#ifndef _JINUE_KERNEL_BOOTMEM_H_
#define _JINUE_KERNEL_BOOTMEM_H_

#include <kernel.h>

struct bootmem_t {
	struct bootmem_t *next;
	physaddr_t addr;
	physsize_t size;
};

typedef struct bootmem_t bootmem_t;

#endif
