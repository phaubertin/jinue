#include <printk.h>

int main(void) {
	volatile int a, b;
	
	/*printk("Hello World!\n");*/
	
	a = 1;
	b = 0;
	a /= b;
	
	while (1) {}
	
	return 0;
}
