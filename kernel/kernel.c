#include <vga.h>

void kernel(void) {
	vga_init();
	vga_print("Kernel started.");
}

