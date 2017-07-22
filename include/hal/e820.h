#ifndef JINUE_HAL_E820_H
#define JINUE_HAL_E820_H

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


bool e820_is_valid(const e820_t *e820_entry);

bool e820_is_available(const e820_t *e820_entry);

const char *e820_type_description(e820_type_t type);

void e820_dump(void);

#endif
