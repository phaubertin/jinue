#ifndef _JINUE_BIOS_H_
#define _JINUE_BIOS_H_

#include <stdbool.h>

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

#endif

