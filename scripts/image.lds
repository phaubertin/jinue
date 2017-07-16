OUTPUT_FORMAT("binary")
TARGET("elf32-i386")
INPUT("bin/boot")
TARGET("binary")
INPUT("bin/setup" "bin/kernel-bin")

SECTIONS {
    boot : {
        bin/boot
    }
    
    setup : {
        bin/setup
    }
    
    kernel : {
        bin/kernel-bin
    }
    
    setup_size_div512   = SIZEOF(setup)  / 512;
    kernel_size_div16   = SIZEOF(kernel) / 16;
}
