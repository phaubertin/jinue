#ifndef JINUE_HAL_INTERRUPT_H
#define JINUE_HAL_INTERRUPT_H

#include <jinue-common/syscall.h>
#include <hal/asm/irq.h>
#include <hal/types.h>
#include <stdint.h>


#define EXCEPTION_GOT_ERR_CODE(irq) \
    (irq == EXCEPTION_ALIGNMENT || \
    (irq >= EXCEPTION_DOUBLE_FAULT && irq <= EXCEPTION_PAGE_FAULT && irq != 9))

extern seg_descriptor_t idt[];


void dispatch_interrupt(unsigned int irq, uintptr_t eip, uint32_t errcode, jinue_syscall_args_t *syscall_args);

#endif

