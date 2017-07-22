#ifndef JINUE_HAL_BOOT_H
#define JINUE_HAL_BOOT_H

#include <hal/asm/boot.h>

#include <jinue/elf.h>
#include <hal/e820.h>
#include <stdbool.h>
#include <stdint.h>


/* The declaration below must match the structure defined at the start of the
 * 32-bit setup code. See boot_info_struct in boot/setup32.asm. */
typedef struct {
    Elf32_Ehdr  *kernel_start;
    uint32_t     kernel_size;
    Elf32_Ehdr  *proc_start;
    uint32_t     proc_size;
    void        *image_start;
    void        *image_top;
    uint32_t     e820_entries;
    e820_t      *e820_map;
    void        *boot_heap;
    void        *boot_end;
    uint32_t     setup_signature;
} boot_info_t;

bool boot_info_check(bool panic_on_failure);

const boot_info_t *get_boot_info(void);

void boot_info_dump(void);

#endif
