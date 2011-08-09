#include <ipc.h>
#include <printk.h>
#include <syscall.h>


void dispatch_ipc(ipc_params_t *ipc_params) {
	if(ipc_params->args.dest == SYSCALL_IPC_REF) {
		dispatch_syscall(ipc_params);
		return;
	}
	
	printk("IPC: dest->%u funct->%u arg1->%u arg2->%u\n",
		ipc_params->args.dest,
		ipc_params->args.funct,
		ipc_params->args.arg1,
		ipc_params->args.arg2);
}
