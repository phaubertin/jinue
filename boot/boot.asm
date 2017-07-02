#include <hal/asm/boot.h>
#include "boot.gen.h"

    times BOOT_E820_ENTRIES -($-$$) db 0

e820_entries:   db 0

    times BOOT_SETUP_SECTS -($-$$) db 0

setup_sects:    db SETUP_SIZE / 512     ; from boot.gen.h
root_flags:     dw 1
sysize:         dd KERNEL_SIZE / 16     ; from boot.gen.h
ram_size:       dw 0
vid_mode:       dw 0xffff
root_dev:       dw 0

signature:
    dw BOOT_SIGNATURE
