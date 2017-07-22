#include <hal/asm/boot.h>

    ; defined by linker - see image.lds linker script
    extern setup_size_div512
    extern kernel_size_div16
    
    times BOOT_E820_ENTRIES -($-$$) db 0

e820_entries:   db 0

    times BOOT_SETUP_SECTS -($-$$) db 0

setup_sects:    db setup_size_div512
root_flags:     dw 1
sysize:         dd kernel_size_div16
ram_size:       dw 0
vid_mode:       dw 0xffff
root_dev:       dw 0

signature:
    dw BOOT_MAGIC
