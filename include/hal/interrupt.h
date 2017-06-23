#ifndef _JINUE_KERNEL_INTERRUPT_H_
#define _JINUE_KERNEL_INTERRUPT_H_

#include <hal/syscall.h>
#include <stdint.h>

void dispatch_interrupt(unsigned int irq, uintptr_t eip, uint32_t errcode, jinue_syscall_args_t *syscall_args);

#endif

