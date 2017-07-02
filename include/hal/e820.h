#ifndef _JINUE_KERNEL_E820_H_
#define _JINUE_KERNEL_E820_H_

#include <hal/asm/e820.h>

#include <stdbool.h>
#include <stdint.h>


typedef uint32_t e820_type_t;

typedef uint64_t e820_size_t;

typedef uint64_t e820_addr_t;

typedef struct {
    e820_addr_t addr;
    e820_size_t size;
    e820_type_t type;
} e820_t;


e820_addr_t e820_get_addr(unsigned int idx);

e820_size_t e820_get_size(unsigned int idx);

e820_type_t e820_get_type(unsigned int idx);

bool e820_is_valid(unsigned int idx);

bool e820_is_available(unsigned int idx);

const char *e820_type_description(e820_type_t type);

void e820_dump(void);

#endif
