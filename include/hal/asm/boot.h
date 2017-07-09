#ifndef JINUE_HAL_ASM_BOOT_H
#define JINUE_HAL_ASM_BOOT_H


#define BOOT_SIGNATURE          0xaa55

#define SETUP_HEADER            0x53726448  /* "HdrS", reversed */

#define BOOT_E820_ENTRIES       0x1e8

#define BOOT_SETUP_SECTS        0x1f1

#define BOOT_SYSIZE             0x1f4

#define BOOT_SETUP              0x200

#define BOOT_E820_MAP           0x2d0

#define BOOT_E820_MAP_END       0xd00


#define BOOT_SETUP_ADDR(x)      ((x) - BOOT_SETUP)

#define BOOT_DATA_STRUCT        BOOT_E820_ENTRIES

#endif
