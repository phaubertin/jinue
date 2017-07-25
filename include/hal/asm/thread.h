#ifndef JINUE_HAL_ASM_THREAD_H
#define JINUE_HAL_ASM_THREAD_H

#include <jinue-common/asm/vm.h>


#define THREAD_CONTEXT_SIZE     PAGE_SIZE

#define THREAD_CONTEXT_MASK     (~(THREAD_CONTEXT_SIZE - 1))

#endif
