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
