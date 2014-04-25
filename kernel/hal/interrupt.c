#include <jinue/syscall.h>
#include <jinue/types.h>
#include <hal/frame_pointer.h>
#include <hal/kernel.h>
#include <interrupt.h>
#include <irq.h>
#include <panic.h>
#include <printk.h>
#include <x86.h>


void dispatch_interrupt(unsigned int irq, uintptr_t eip, uint32_t errcode, syscall_params_t *syscall_params) {
    /* entering (or re-entering) the kernel */
    ++in_kernel;
    
    /* exceptions */
    if(irq < IDT_FIRST_IRQ) {
        printk("EXCEPT: %u cr2=0x%x errcode=0x%x eip=0x%x\n", irq, get_cr2(), errcode, eip);
        
        /* never returns */
        panic("caught exception");
    }
    
    /* slow system call method */
    if(irq == SYSCALL_IRQ) {
        hal_syscall_dispatch(syscall_params);
    }
    else {        
        printk("INTR: irq %u (vector %u)\n", irq - IDT_FIRST_IRQ, irq);
    }
    
    /* leaving (or returning to) the kernel */
    --in_kernel;
}
