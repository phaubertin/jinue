#ifndef JINUE_HAL_BOOTMEM_H
#define JINUE_HAL_BOOTMEM_H

#include <hal/e820.h>
#include <types.h>


struct bootmem_t {
    struct bootmem_t *next;
    pfaddr_t addr;
    uint32_t count;
};

typedef struct bootmem_t bootmem_t;

/** kernel memory map */
extern bootmem_t *ram_map;

/** available memory map (allocator) */
extern bootmem_t *bootmem_root;

/** current top of boot heap */
extern void *boot_heap;


void new_ram_map_entry(pfaddr_t addr, uint32_t count, bootmem_t **head);

void apply_mem_hole(e820_addr_t hole_start, e820_addr_t hole_end, bootmem_t **head);

void bootmem_init(void);

bootmem_t *bootmem_get_block(void);

#endif
