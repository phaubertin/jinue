#ifndef JINUE_HAL_ASM_E820_H
#define JINUE_HAL_ASM_E820_H

#include <hal/asm/boot.h>

#define E820_RAM            1

#define E820_RESERVED       2

#define E820_ACPI           3

#define E820_SMAP           0x534d4150

#define E820_MAX_SIZE       (BOOT_E820_MAP_END - BOOT_E820_MAP)

#endif
