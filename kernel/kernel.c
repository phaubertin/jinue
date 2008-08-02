#include <ascii.h>
#include <bios.h>
#include <vga.h>
#include <printk.h>

void kernel(void) {
	unsigned int idx;
	
	vga_init();
	
	printk("Kernel started.\n");
	
	idx = 0;
	printk("Dump of the BIOS memory map:\n");
	printk("%caddress  size     type\n", CHAR_HT);
	while( e820_is_valid(idx) ) {
		printk("%c%c%x %x %s\n",
			e820_is_available(idx)?'*':' ',
			CHAR_HT,
			e820_get_addr(idx),
			e820_get_size(idx),
			e820_type_description( e820_get_type(idx) ) );
		++idx;
	}
}

