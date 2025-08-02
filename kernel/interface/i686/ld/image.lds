/*
 * Copyright (C) 2019-2025 Philippe Aubertin.
 * All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the author nor the names of other contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
 
#include <kernel/interface/i686/asm/boot.h>

OUTPUT_FORMAT("binary")

TARGET("elf32-i386")
INPUT("interface/i686/setup/setup16-nasm.o")
INPUT("interface/i686/setup/setup32.o")

/* Even though the next two input files are ELF binaries, we want to insert them
 * whole into the kernel image, hence why the TARGET is specified as binary. Both
 * are already linked. */
TARGET("binary")
INPUT("../userspace/loader/loader-stripped")
INPUT("interface/i686/kernel-stripped")

SECTIONS {
    /* The intent of this linker script is to create a kernel image that conforms
     * to Linux's 16-bit and 32-bit boot protocol on x86. See the following
     * documentation file in the Linux kernel source code distribution for detail:
     *              Documentation/x86/boot.txt
     *
     * (or simply search for boot.txt online.)
     *
     * The following section is the real-mode setup code. Consistent with the
     * boot protocol, it is loaded by the bootloader separately from the rest
     * of the kernel image, usually at 9000:0 (i.e. 0x90000 linear address).
     *
     * In addition to early initialization code, this section also contains a
     * header at a known offset with information for the bootloader. Some
     * symbols defined in this linker script are used to create this header.
     * 
     * Load address of this section:    9000:0 (usually)
     * Run address:                     Same as load address
     * Needed symbols:                  setup_size_div512
     *                                  kernel_size_div16 */
    setup16 : {
        interface/i686/setup/setup16-nasm.o(.text)
    }
    
    /* Everything below this point is loaded consecutively starting at 0x100000
     * (BOOT_SETUP32_ADDR) and runs at the same address plus KERNEL_BASE, with
     * some exceptions for the 32-bit setup code - see below.
     *
     * The section that follows is the 32-bit setup code. Its load address (LMA)
     * has to be explicitly set here to ADDR(setup16) + SIZEOF(setup16) to
     * ensure it is consecutive to the previous section in the output binary
     * file. If this is not done, the linker inserts almost 1MB of padding
     * (0x100000 - SIZEOF(setup16)) between these two sections.
     *
     * Consistent with the 32-bit boot protocol, this first section of the image
     * loaded at 0x100000 is a single page of code that contains the 32-bit
     * setup code. Furthermore, the 16-bit setup code above sets things up as
     * the bootloader would have done had the 32-bit protocol been used and
     * then jumps to the 32-bit setup code entry point. Long story short,
     * this 32-bit setup code is used irrespective of whether the 16-bit or
     * 32-bit boot protocol is used.
     *
     * This setup code does mainly three things. First, it maps the kernel image
     * at KERNEL_BASE and enables paging. This means the whole 32-bit kernel
     * runs above JINUE_KLIMIT, except for the early parts of this setup code
     * needed to set up the mapping. Second, it copies the BIOS memory map and
     * kernel command line for later use by the kernel. Finally, it gathers
     * information about the location in memory of various parts of the kernel
     * and creates a structure with this information for the kernel.
     *
     * Load address of this section:    0x100000 (BOOT_SETUP32_ADDR)
     * Run address:                     KERNEL_BASE
     * Needed symbols:                  kernel_start
     *                                  kernel_size
     *                                  loader_start
     *                                  loader_size
     *                                  image_start */
    setup32 BOOT_SETUP32_ADDR : AT(ADDR(setup16) + SIZEOF(setup16)) {
        image_start = .;
        interface/i686/setup/setup32.o(.text)
    }
    
    /* The next section must start at a specific offset (BOOT_SETUP32_SIZE) from
     * the start of the previous section. Here, we set the location counter
     * accordingly. Padding is added by the linker as needed. */
    . = BOOT_SETUP32_ADDR + BOOT_SETUP32_SIZE;
    
    /* This next section is the kernel proper. It is an ELF binary complete with
     * headers, symbols and all. It is linked using another linker script - see
     * scripts/kernel.lds. The 32-bit setup code uses the kernel_start symbol
     * defined here to find the kernel and knows just enough of the ELF format
     * to be able to find its entry point address. */
    kernel_start = .;
    
    kernel : {
        interface/i686/kernel-stripped
        kernel_end = ABSOLUTE(.);
        
        . = ALIGN(PAGE_SIZE);
    }
    
    /* The section that follows is the ELF binary for the user space loader. */
    loader_start = .;
    
    loader : {
        ../userspace/loader/loader-stripped
        loader_end = ABSOLUTE(.);
        
        . = ALIGN(PAGE_SIZE);
    }
    
    image_top = .;
    
    kernel_size = kernel_end - kernel_start;
    loader_size = loader_end - loader_start;
    
    /* The value of these two symbols is placed in the header located in the
     * 16-bit setup code section as specified by the boot protocol.
     * 
     * The first one is the size of the 16-bit setup code itself in 512-byte
     * sectors. Consistent with the boot protocol specification, we must subtract
     * one sector as we must not count the first sector, which is the legacy boot
     * sector.
     * 
     * The second symbol is the total size of the kernel image loaded at 0x100000
     * (i.e. everything except the 16-bit setup code section) in 16-byte units. */
    setup_size_div512   = SIZEOF(setup16) / 512 - 1;
    kernel_size_div16   = (image_top - BOOT_SETUP32_ADDR) / 16;
}
