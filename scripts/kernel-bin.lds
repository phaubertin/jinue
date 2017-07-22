OUTPUT_FORMAT("binary")

TARGET("binary")
INPUT("proc/proc-stripped")
INPUT("kernel/kernel-stripped")

TARGET("elf32-i386")
INPUT("boot/setup32.o")


SECTIONS {
    setup32 0x100000 : {
        boot/setup32.o
        
        . = ALIGN(4096);
    }
    
    kernel_start = .;
    
    kernel : {
        kernel/kernel-stripped
        kernel_end = ABSOLUTE(.);
        
        . = ALIGN(4096);
    }
    
    proc_start = .;
    
    proc : {
        proc/proc-stripped
        proc_end = ABSOLUTE(.);
        
        . = ALIGN(4096);
    }
    
    kernel_top = .;
    
    kernel_size = kernel_end - kernel_start;
    proc_size   = proc_end   - proc_start;
}
