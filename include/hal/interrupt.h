#ifndef JINUE_HAL_INTERRUPT_H
#define JINUE_HAL_INTERRUPT_H

#include <jinue-common/syscall.h>
#include <stdint.h>


void dispatch_interrupt(unsigned int irq, uintptr_t eip, uint32_t errcode, jinue_syscall_args_t *syscall_args);

#endif

