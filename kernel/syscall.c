#include <ipc.h>
#include <printk.h>
#include <stddef.h>
#include <syscall.h>
#include <vga.h>


int syscall_method;


void dispatch_syscall(ipc_params_t *ipc_params) {
	unsigned int funct;
	unsigned int arg1;
	unsigned int arg2;
	
	funct = ipc_params->args.funct;
	arg1  = ipc_params->args.arg1;
	arg2  = ipc_params->args.arg2;
	
	ipc_params->ret.errno    = 0;
	ipc_params->ret.perrno   = NULL;
	ipc_params->ret.reserved = 0;	
	
	switch(funct) {
	
	case SYSCALL_FUNCT_SYSCALL_METHOD:
		ipc_params->ret.val = syscall_method;
		break;		
		
	case SYSCALL_FUNCT_VGA_PUTC:
		/** TODO: permission check */
		vga_putc((char)arg1);
		break;
	
	case SYSCALL_FUNCT_VGA_PUTS:
		/** TODO: permission check, sanity check */
		vga_printn((char *)arg1, arg2);
		break;
	
	default:
		printk("SYSCALL: funct->%u arg1->%u arg2->%u\n", funct,	arg1, arg2);
	}	
}
