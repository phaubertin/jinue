#ifndef _JINUE_KERNEL_BOOT_H_
#define _JINUE_KERNEL_BOOT_H_

#include <hal/asm/boot.h>

#include <stdint.h>

#pragma pack(push, 1)
typedef struct {
    uint8_t     e820_entries;
    uint8_t     unused[BOOT_SETUP_SECTS - BOOT_E820_ENTRIES - 1];
    uint8_t     setup_sects;
    uint16_t    root_flags;
    uint32_t    sysize;
    uint16_t    ram_size;
    uint16_t    vid_mode;
    uint16_t    root_dev;
    uint16_t    signature;
} boot_t;
#pragma pack(pop)


boot_t *get_boot_data(void);

#endif
