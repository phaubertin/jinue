OUTPUT_FORMAT("binary")

SECTIONS {
	kernel 0x100000 : {
		*(.text)
		*(.data)
		*(.bss)
		
		. = ALIGN(16);
	}
	
	/DISCARD/ : {
		*(.*)
	}
}
