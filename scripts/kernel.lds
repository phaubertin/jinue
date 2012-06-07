OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(start)

SECTIONS {
	.text 0x100000 : {
		hal/asm/startup.o(.text)
		*(.text)
		*(.text.*)
		*(.rodata)
		*(.rodata.*)
		
		. = ALIGN(16);
	}
	
	.data : {
		*(.data)
		*(.data.*)
		
		. = ALIGN(16);
	}
	
	.bss : {
		*(.bss)
		*(.bss.*)
		
		. = ALIGN(16);
	}
}

