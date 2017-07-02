#ifndef _JINUE_KERNEL_HAL_ASM_E820_H_
#define _JINUE_KERNEL_HAL_ASM_E820_H_

#include <hal/asm/boot.h>

#define E820_RAM            1

#define E820_RESERVED       2

#define E820_ACPI           3

#define E820_SMAP           0x534d4150

#define E820_MAX_SIZE       (BOOT_E820_MAP_END - BOOT_E820_MAP)

#endif
