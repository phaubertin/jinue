#include <hal/startup.h>
#include <hal/vga.h>
#include <debug.h>
#include <printk.h>


void panic(const char *message) {	
	unsigned int color;
	
	color = vga_get_color();
	vga_set_color(VGA_COLOR_RED);
	
	printk("KERNEL PANIC: %s\n", message);
	
	vga_set_color(color);	

	dump_call_stack();
	halt();
}
