#ifndef JINUE_HAL_BOOT_H
#define JINUE_HAL_BOOT_H

#include <hal/asm/boot.h>

#include <hal/types.h>


bool boot_info_check(bool panic_on_failure);

const boot_info_t *get_boot_info(void);

void boot_info_dump(void);

#endif
