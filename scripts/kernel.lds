/*
 * Copyright (C) 2017 Philippe Aubertin.
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

#include <jinue-common/asm/vm.h>
#include <hal/asm/boot.h>

OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)

SECTIONS {
    . = KLIMIT + BOOT_SETUP32_ADDR + BOOT_SETUP32_SIZE + SIZEOF_HEADERS;
    .text : {
        *(.text)
        *(.text.*)
    }
    
    .rodata : {
        *(.rodata)
        *(.rodata.*)
        
        /* The kernel ELF binary file is loaded in memory (i.e. the whole file
         * is copied as-is) and then executed with the assumption that memory
         * offsets and file offsets are the same. The build process must ensure
         * that this assumption holds.
         * 
         * For this to work, we must ensure that the end of the text section and
         * the start of the data section are on different pages. */
        . = ALIGN(PAGE_SIZE);
    }
    
    .data : {
        *(.data)
        *(.data.*)
        
        /* Put uninitialized data in the .data section to ensure space is
         * actually reserved for them in the file. */
        *(.bss)
        *(.bss.*)
        
        . = ALIGN(16);
    }
    
    .eh_frame           : { *(.eh_frame) }
    .shstrtab           : { *(.shstrtab) }
    .symtab             : { *(.symtab) }
    .strtab             : { *(.strtab) }
    .comment            : { *(.comment) }
    .note.gnu.build-id  : { *(.note.gnu.build-id)}
}
