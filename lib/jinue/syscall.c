#include <jinue/syscall.h>
#include <stddef.h>


syscall_stub_t syscall = NULL;


int get_syscall_method(void) {
	return syscall_intr(SYSCALL_IPC_REF, SYSCALL_FUNCT_SYSCALL_METHOD, NULL, NULL);
}

void set_syscall_method(void) {
	int method = get_syscall_method();
	syscall = syscall_stubs[method];
}
