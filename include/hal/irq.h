#ifndef _JINUE_KERNEL_HAL_IRQ_H_
#define _JINUE_KERNEL_HAL_IRQ_H_

#include <hal/descriptors.h>

#include <hal/asm/irq.h>

#define EXCEPTION_GOT_ERR_CODE(irq) \
    (irq == EXCEPTION_ALIGNMENT || \
    (irq >= EXCEPTION_DOUBLE_FAULT && irq <= EXCEPTION_PAGE_FAULT && irq != 9))

extern seg_descriptor_t idt[];

#endif

