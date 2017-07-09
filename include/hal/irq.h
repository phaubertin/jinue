#ifndef JINUE_HAL_IRQ_H
#define JINUE_HAL_IRQ_H

#include <hal/descriptors.h>

#include <hal/asm/irq.h>

#define EXCEPTION_GOT_ERR_CODE(irq) \
    (irq == EXCEPTION_ALIGNMENT || \
    (irq >= EXCEPTION_DOUBLE_FAULT && irq <= EXCEPTION_PAGE_FAULT && irq != 9))

extern seg_descriptor_t idt[];

/* do not call - used by new user threads to "return" to user space for the
 * first time. See thread_page_create(). */
void return_from_interrupt(void);

#endif

