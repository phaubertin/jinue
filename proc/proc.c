#include <jinue/syscall.h>
#include <printk.h>


int main(void) {
	int method;
	
	method = set_syscall_method();
	
	printk("Process manager started.\n");
	
	printk("Using system call method '%s'.\n", syscall_stub_names[method]);

	while (1) {}
	
	return 0;
}
