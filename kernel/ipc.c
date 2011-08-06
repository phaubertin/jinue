#include <ipc.h>

void dispatch_ipc(ipc_ret_t dest, unsigned int funct, unsigned int arg1, unsigned int arg2) {
	if(dest == 0) {
		dispatch_syscall(funct, arg1, arg2);
	}
}

void dispatch_syscall(unsigned int funct, unsigned int arg1, unsigned int arg2) {}
