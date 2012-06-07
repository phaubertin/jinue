#ifndef _JINUE_KERNEL_BOOT_H_
#define _JINUE_KERNEL_BOOT_H_

#include <stdint.h>


#define BOOT_SIGNATURE 0xaa55

#define BOOT_MAGIC     0xcafef00d

#define SETUP_HEADER   0x53726448  /* "HdrS", reversed */


#pragma pack(push, 1)
typedef struct {
	uint32_t  magic;
	uint8_t   setup_sects;
	uint16_t  root_flags;
	uint32_t  sysize;
	uint16_t  ram_size;
	uint16_t  vid_mode;
	uint16_t  root_dev;
	uint16_t  signature;
} boot_t;
#pragma pack(pop)


boot_t *get_boot_data(void);

#endif
