OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(start)

SECTIONS {
	.text 0x100000 : {
		startup.o(.text)
		*(.text)
		*(.text.*)
		
		. = ALIGN(16);
	}
	
	_etext = .;
	
	.data : {
		*(.data)
		*(.data.*)
		*(.rodata)
		*(.rodata.*)
		
		. = ALIGN(16);
	}
	
	_edata = .;
	
	.bss : {
		*(.bss)
		*(.bss.*)
		
		. = ALIGN(16);
	}
	
	_end = .;
}

