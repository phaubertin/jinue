#include <hal/interrupt.h>
#include <hal/x86.h>
#include <panic.h>
#include <printk.h>
#include <syscall.h>
#include <types.h>


void dispatch_interrupt(unsigned int irq, uintptr_t eip, uint32_t errcode, jinue_syscall_args_t *syscall_args) {
    /* exceptions */
    if(irq < IDT_FIRST_IRQ) {
        printk("EXCEPT: %u cr2=0x%x errcode=0x%x eip=0x%x\n", irq, get_cr2(), errcode, eip);
        
        /* never returns */
        panic("caught exception");
    }
    
    /* slow system call method */
    if(irq == SYSCALL_IRQ) {
        dispatch_syscall(syscall_args);
    }
    else {
        printk("INTR: irq %u (vector %u)\n", irq - IDT_FIRST_IRQ, irq);
    }
}
