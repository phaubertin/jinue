#include <jinue/syscall.h>
#include <printk.h>


int main(void) {
	set_syscall_method();
	
	printk("Process manager started.\n");

	while (1) {}
	
	return 0;
}
