OUTPUT_FORMAT("binary")
TARGET("elf32-i386")
INPUT("boot/boot.o")
TARGET("binary")
INPUT("boot/setup.bin" "bin/kernel-bin")

SECTIONS {
    boot : {
        boot/boot.o
    }
    
    setup : {
        boot/setup.bin
    }
    
    kernel : {
        bin/kernel-bin
    }
    
    setup_size_div512   = SIZEOF(setup)  / 512;
    kernel_size_div16   = SIZEOF(kernel) / 16;
}
