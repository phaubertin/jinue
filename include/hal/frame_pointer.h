#ifndef _JINUE_KERNEL_FPOINTER_H_
#define _JINUE_KERNEL_FPOINTER_H_

#include <jinue/types.h>

addr_t get_fpointer(void);

addr_t get_caller_fpointer(addr_t fptr);

addr_t get_ret_addr(addr_t fptr);

addr_t get_program_counter(void);

#endif
