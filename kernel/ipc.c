#include <jinue/ipc.h>
#include <jinue/syscall.h>
#include <ipc.h>
#include <printk.h>
#include <vga.h>

void dispatch_ipc(ipc_ref_t dest, unsigned int funct, unsigned int arg1, unsigned int arg2) {
	if(dest == SYSCALL_IPC_REF) {
		dispatch_syscall(funct, arg1, arg2);
	}
	
	else {
		printk("IPC: dest->%u funct->%u arg1->%u arg2->%u\n", dest, funct, arg1, arg2);
	}
}

void dispatch_syscall(unsigned int funct, unsigned int arg1, unsigned int arg2) {
	switch(funct) {
		
	case SYSCALL_FUNCT_VGA_PUTC:
		/** TODO: permission check */
		vga_putc((char)arg1);
		break;
	
	case SYSCALL_FUNCT_VGA_PUTS:
		/** TODO: permission check, sanity check */
		vga_printn((char *)arg1, arg2);
		break;
	
	default:
		printk("SYSCALL: funct->%u arg1->%u arg2->%u\n", funct, arg1, arg2);
	}	
}
