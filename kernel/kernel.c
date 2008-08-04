#include <alloc.h>
#include <kernel.h>
#include <printk.h>
#include <vga.h>

void kernel(void) {
	kinit();	
	idle();
	
	panic("idle() returned.");	
}

void kinit(void) {	
	vga_init();	
	printk("Kernel started.\n");
	
	alloc_init();
}

void idle(void) {
	while(1) {}
}

