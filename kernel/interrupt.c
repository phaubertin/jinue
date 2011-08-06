#include <interrupt.h>
#include <irq.h>
#include <panic.h>
#include <printk.h>
#include <x86.h>

unsigned int syscall_irq;

void dispatch_interrupt(unsigned int irq, ipc_params_t *ipc_params) {
	/* slow system call/ipc mechanism */
	if(syscall_irq != 0 && irq == syscall_irq) {
		dispatch_ipc(ipc_params->dest, ipc_params->funct, ipc_params->arg1, ipc_params->arg2);
	}
	
	if(irq < IDT_FIRST_IRQ) {
		printk("EXCEPT: %u cr2=%x\n", irq, get_cr2());
		panic("caught exception");
	}
	
	printk("INTR: irq %u (vector %u)\n", irq - IDT_FIRST_IRQ, irq);
}
