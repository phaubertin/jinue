#ifndef _JINUE_KERNEL_IRQ_H_
#define _JINUE_KERNEL_IRQ_H_

#include <descriptors.h>


#define IDT_VECTOR_COUNT	256

#define IDT_FIRST_IRQ		 32

#define IDT_IRQ_COUNT		 (IDT_VECTOR_COUNT - IDT_FIRST_IRQ)


extern seg_descriptor_t idt[];

#endif

