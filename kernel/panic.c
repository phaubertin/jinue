#include <startup.h>
#include <printk.h>

void panic(const char *message) {
	printk("KERNEL PANIC: %s\n", message);
	halt();
}

