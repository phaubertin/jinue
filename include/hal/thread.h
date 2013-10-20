#ifndef _JINUE_KERNEL_HAL_THREAD_H_
#define _JINUE_KERNEL_HAL_THREAD_H_

#include <jinue/types.h>
#include <hal/vm.h>


void thread_start(addr_t entry, addr_t user_stack);

int thread_switch(addr_t vstack, pfaddr_t pstack, unsigned int flags, pte_t *pte, int next);

#endif

