#ifndef _JINUE_KERNEL_INTERRUPT_H_
#define _JINUE_KERNEL_INTERRUPT_H_

#include <ipc.h>

extern unsigned int syscall_irq;

void dispatch_interrupt(unsigned int irq, ipc_params_t *ipc_params);

#endif

