#include <jinue-common/asm/vm.h>
#include <hal/asm/boot.h>

OUTPUT_FORMAT("binary")

TARGET("binary")
INPUT("proc/proc-stripped")
INPUT("kernel/kernel-stripped")

TARGET("elf32-i386")
INPUT("boot/setup32.o")


SECTIONS {
    setup32 BOOT_SETUP32_ADDR : {
        boot/setup32.o
    }
    
    . = BOOT_SETUP32_ADDR + BOOT_SETUP32_SIZE;
    kernel_start = .;
    
    kernel : {
        kernel/kernel-stripped
        kernel_end = ABSOLUTE(.);
        
        . = ALIGN(PAGE_SIZE);
    }
    
    proc_start = .;
    
    proc : {
        proc/proc-stripped
        proc_end = ABSOLUTE(.);
        
        . = ALIGN(PAGE_SIZE);
    }
    
    kernel_top = .;
    
    kernel_size = kernel_end - kernel_start;
    proc_size   = proc_end   - proc_start;
}
