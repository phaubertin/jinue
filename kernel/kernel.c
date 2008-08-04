#include <alloc.h>
#include <boot.h>
#include <kernel.h>
#include <panic.h>
#include <printk.h>
#include <vga.h>

void kernel(void) {
	kinit();	
	idle();
	
	panic("idle() returned.");	
}

void kinit(void) {
	boot_t *boot;
	unsigned int kernel_size;
	
	/* say hello */
	vga_init();	
	printk("Kernel started.\n");
	
	/* find out kernel size */
	boot = get_boot_data();
	kernel_size = boot->sysize * 16;
	
	printk("Kernel size is %u bytes.\n", kernel_size);	
	
	/* initialize allocator */
	alloc_init();
}

void idle(void) {
	while(1) {}
}

