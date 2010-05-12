#include <boot.h>   /* includes stdbool.h */
#include <kernel.h> /* includes stddef.h */
#include <panic.h>
#include <printk.h>

e820_t *e820_map;
addr_t  boot_data;

physaddr_t e820_get_addr(unsigned int idx) {
	return e820_map[idx].addr;
}

physsize_t e820_get_size(unsigned int idx) {
	return e820_map[idx].size;
}

e820_type_t e820_get_type(unsigned int idx) {
	return e820_map[idx].type;
}

bool e820_is_valid(unsigned int idx) {
	return (e820_map[idx].size != 0);
}

bool e820_is_available(unsigned int idx) {
	return (e820_map[idx].type == E820_RAM);
}

const char *e820_type_description(e820_type_t type) {
	switch(type) {
	
	case E820_RAM:
		return "available";
	
	case E820_RESERVED:
		return "unavailable/reserved";
	
	case E820_ACPI:
		return "unavailable/acpi";
	
	default:
		return "unavailable/other";
	}	
}

boot_t *get_boot_data(void) {
	boot_t *boot;
	
	boot = (boot_t *)boot_data;
		
	if(boot->signature != BOOT_SIGNATURE) {
		panic("bad boot sector signature.");
	}
	
	if(boot->magic != BOOT_MAGIC) {
		panic("bad boot sector magic.");
	}
	
	return boot;
}

void e820_dump(void) {
	unsigned int idx;
	
	printk("Dump of the BIOS memory map:\n");
	printk("  address  size     type\n");

	for(idx = 0; e820_is_valid(idx); ++idx) {		
		printk("%c %q %q %s\n",
			e820_is_available(idx)?'*':' ',
			e820_get_addr(idx),
			e820_get_size(idx),
			e820_type_description( e820_get_type(idx) )
		);
	}
}
