#include <stddef.h>
#include <debug.h>
#include <frame_pointer.h>
#include <kernel.h>
#include <printk.h>


void dump_call_stack(void) {
	addr_t 				 ret;
	addr_t 				 fptr;
	debugging_symbol_t	*sym;
	
	printk("Call stack dump:\n");
	fptr = get_fpointer();
	
	while(fptr != NULL) {
		ret = get_ret_addr(fptr);
		if(ret == NULL) {
			break;
		}
		
		/* assume e8 xx xx xx xx for call instruction encoding */
		ret -= 5;
		
		sym = get_debugging_symbol(ret);
		if(sym == NULL) {
			printk("\t0x%x (unknown)\n", ret);			
		}
		else {
			printk("\t0x%x (%s+%u)\n", ret, sym->name, ret - sym->addr);
		}
		
		fptr = get_caller_fpointer(fptr);
	}
}

debugging_symbol_t *get_debugging_symbol(addr_t addr) {
	debugging_symbol_t *sym;
	
	if(addr >= &kernel_end) {
		return NULL;
	}
	
	sym = debugging_symbols_table;
	
	if(addr < sym->addr) {
		return NULL;
	}
	
	while(sym->addr) {
		if(addr < sym->addr) {
			return sym - 1;
		}
		++sym;
	}
	
	return sym - 1;
}
