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


/** kernel memory map */
extern bootmem_t *ram_map;

/** available memory map (allocator) */
extern bootmem_t *bootmem_root;

/** current map entry (allocator) */
extern bootmem_t *bootmem_cur;

/** current top of boot heap */
extern addr_t boot_heap;


physaddr_t bootmem_alloc_page(void);

void new_ram_map_entry(physaddr_t addr, physsize_t size, bootmem_t **head);

void apply_mem_hole(bootmem_t **ptr, physaddr_t hole_addr, physsize_t hole_size, bootmem_t **head);

void bootmem_set_cur(void);

void bootmem_init(void);

bootmem_t *bootmem_get_block(void);

#endif
