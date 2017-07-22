#ifndef JINUE_HAL_ASM_BOOT_H
#define JINUE_HAL_ASM_BOOT_H

#include <jinue/asm/vm.h>

       

#define BOOT_E820_ENTRIES       0x1e8

#define BOOT_SETUP_SECTS        0x1f1

#define BOOT_SYSIZE             0x1f4

#define BOOT_SIGNATURE          0x1fe

#define BOOT_MAGIC              0xaa55

#define BOOT_SETUP              0x200

#define BOOT_SETUP_HEADER       0x202

#define BOOT_SETUP_MAGIC        0x53726448  /* "HdrS", reversed */

#define BOOT_E820_MAP           0x2d0

#define BOOT_E820_MAP_END       0xd00

#define BOOT_E820_MAP_SIZE      (BOOT_E820_MAP_END - BOOT_E820_MAP)

#define BOOT_SETUP32_ADDR       0x100000

#define BOOT_SETUP32_SIZE       PAGE_SIZE

#define BOOT_SETUP_ADDR(x)      ((x) - BOOT_SETUP)

#define BOOT_DATA_STRUCT        BOOT_E820_ENTRIES

#define BOOT_STACK_SIZE         (2 * PAGE_SIZE)

#endif
