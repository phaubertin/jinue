#include <jinue/syscall.h>
#include <interrupt.h>
#include <irq.h>
#include <panic.h>
#include <printk.h>
#include <x86.h>


void dispatch_interrupt(unsigned int irq, ipc_params_t *ipc_params) {
	unsigned long errcode;
	unsigned long eip;
	unsigned long *return_state;
	
	/* exceptions */
	if(irq < IDT_FIRST_IRQ) {
		return_state = (unsigned long *)ipc_params;
		
		if( EXCEPTION_GOT_ERR_CODE(irq) ) {
			errcode = return_state[12];
			eip     = return_state[13];
		}
		else {
			errcode = 0;
			eip     = return_state[12];
		}		
		
		printk("EXCEPT: %u cr2=0x%x errcode=0x%x eip=0x%x\n", irq, get_cr2(), errcode, eip);
		
		/* never returns */
		panic("caught exception");
	}
	
	/* slow system call/ipc mechanism */
	if(irq == SYSCALL_IRQ) {
		dispatch_ipc(ipc_params->dest, ipc_params->funct, ipc_params->arg1, ipc_params->arg2);
		return;
	}
		
	printk("INTR: irq %u (vector %u)\n", irq - IDT_FIRST_IRQ, irq);
}
