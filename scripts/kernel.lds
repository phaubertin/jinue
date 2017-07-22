OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)

SECTIONS {
    /* TODO process this file with C preprocessor and use constant names below */
    . = 0x100000 + 0x1000 + SIZEOF_HEADERS;
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
        . = ALIGN(4096);
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
