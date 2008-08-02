#include <vga.h>
#include <printk.h>

void kernel(void) {
	vga_init();
	printk("Kernel started.\n");
}

