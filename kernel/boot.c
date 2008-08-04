#include <boot.h>   /* includes stdbool.h */
#include <kernel.h> /* includes stddef.h */
#include <panic.h>

e820_t *e820_map;
addr_t  boot_setup_addr;

addr_t e820_get_addr(unsigned int idx) {
	return (addr_t)(unsigned long)e820_map[idx].addr;
}

size_t e820_get_size(unsigned int idx) {
	return (size_t)e820_map[idx].size;
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
	
	boot = (boot_t *)( boot_setup_addr - sizeof(boot_t) );
		
	if(boot->signature != BOOT_SIGNATURE) {
		panic("bad boot sector signature.");
	}
	
	if(boot->magic != BOOT_MAGIC) {
		panic("bad boot sector magic.");
	}
	
	return boot;
}

