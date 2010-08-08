#include <printk.h>

volatile int g_var;

int main(void) {
	g_var += 42;
	
	printk("Hello World!\n");
	
	return 0;
}
