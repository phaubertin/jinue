#ifndef _JINUE_KERNEL_DEBUG_H_
#define _JINUE_KERNEL_DEBUG_H_

#include <kernel.h>


typedef struct {
	addr_t addr;
	char *type;
	char *name;
} debugging_symbol_t;


/* this array is defined in the automatically generated symbols.c file */
extern debugging_symbol_t debugging_symbols_table[];


void dump_call_stack(void);

debugging_symbol_t *get_debugging_symbol(addr_t addr);


#endif

