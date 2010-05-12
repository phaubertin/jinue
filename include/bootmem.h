#ifndef _JINUE_KERNEL_BOOTMEM_H_
#define _JINUE_KERNEL_BOOTMEM_H_

#include <kernel.h>

struct bootmem_t {
	struct bootmem_t *next;
	physaddr_t addr;
	physsize_t size;
};

typedef struct bootmem_t bootmem_t;

#define apply_mem_hole_range(ptr, start, end, head) \
	apply_mem_hole((ptr), (start), ((end) - (start)), (head) )


void new_ram_map_entry(physaddr_t addr, physsize_t size, bootmem_t **head);

void apply_mem_hole(bootmem_t **ptr, physaddr_t hole_addr, physsize_t hole_size, bootmem_t **head);

void bootmem_init(void);

#endif
