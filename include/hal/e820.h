#ifndef JINUE_HAL_E820_H
#define JINUE_HAL_E820_H

#include <hal/asm/e820.h>

#include <hal/types.h>


bool e820_is_valid(const e820_t *e820_entry);

bool e820_is_available(const e820_t *e820_entry);

const char *e820_type_description(e820_type_t type);

void e820_dump(void);

#endif
