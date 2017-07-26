#ifndef JINUE_HAL_FPOINTER_H
#define JINUE_HAL_FPOINTER_H

#include <hal/types.h>


addr_t get_fpointer(void);

addr_t get_caller_fpointer(addr_t fptr);

addr_t get_ret_addr(addr_t fptr);

addr_t get_program_counter(void);

#endif
