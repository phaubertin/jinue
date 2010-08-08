OUTPUT_FORMAT("binary")
TARGET("binary")
INPUT("proc/proc")
TARGET("elf32-i386")
INPUT("kernel/kernel")

SECTIONS {
	kernel 0x100000 : {
		kernel/kernel(.text)
		kernel/kernel(.data)
		kernel/kernel(.bss)
		
		kernel_end = ABSOLUTE(.);
		
		. = ALIGN(16);
	}
	
	. = ALIGN(4096);
	proc_elf = .;
	
	proc : {
		proc/proc
		
		proc_elf_end = ABSOLUTE(.);
		. = ALIGN(16);
	}
	
	/DISCARD/ : {
		*(.*)
	}
}
