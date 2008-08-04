#ifndef _JINUE_BOOT_H_
#define _JINUE_BOOT_H_

#include <kernel.h>
#include <stdbool.h>

#define BOOT_SIGNATURE 0xaa55
#define BOOT_MAGIC     0xcafef00d
#define SETUP_HEADER   0x53726448  /* "HdrS", reversed */

#define E820_RAM      1
#define E820_RESERVED 2
#define E820_ACPI     3

typedef unsigned long long e820_addr_t;
typedef unsigned long long e820_size_t;
typedef unsigned long e820_type_t;

typedef struct {
	e820_addr_t addr;
	e820_size_t size;
	e820_type_t type;
} e820_t;

#pragma pack(push, 1)
typedef struct {
	unsigned long  magic;
	unsigned char  setup_sects;
	unsigned short root_flags;
	unsigned long  sysize;
	unsigned short ram_size;
	unsigned short vid_mode;
	unsigned short root_dev;
	unsigned short signature;
} boot_t;
#pragma pack(pop)

addr_t e820_get_addr(unsigned int idx);

size_t e820_get_size(unsigned int idx);

e820_type_t e820_get_type(unsigned int idx);

bool e820_is_valid(unsigned int idx);

bool e820_is_available(unsigned int idx);

const char *e820_type_description(e820_type_t type);

boot_t *get_boot_data(void);

#endif

