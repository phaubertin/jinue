#ifndef _JINUE_KERNEL_INTERRUPT_H_
#define _JINUE_KERNEL_INTERRUPT_H_

#include <hal/syscall.h>


void dispatch_interrupt(unsigned int irq, syscall_params_t *syscall_params);

#endif

