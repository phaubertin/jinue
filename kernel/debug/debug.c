#include <hal/frame_pointer.h>
#include <hal/kernel.h>
#include <stddef.h>
#include <debug.h>
#include <printk.h>


void dump_call_stack(void) {
    addr_t               return_addr;
    addr_t               fptr;
    debugging_symbol_t  *sym;
    
    printk("Call stack dump:\n");
    fptr = get_fpointer();
    
    while(fptr != NULL) {
        return_addr = get_ret_addr(fptr);
        if(return_addr == NULL) {
            break;
        }
        
        /* assume e8 xx xx xx xx for call instruction encoding */
        return_addr -= 5;
        
        sym = get_debugging_symbol(return_addr);
        if(sym == NULL) {
            printk("\t0x%x (unknown)\n", return_addr);
        }
        else {
            const char *name = sym->name;

            if(name == NULL) {
                name = "[unknown]";
            }

            printk("\t0x%x (%s+%u)\n", return_addr, name, return_addr - sym->addr);
        }
        
        fptr = get_caller_fpointer(fptr);
    }
}

debugging_symbol_t *get_debugging_symbol(addr_t addr) {
    debugging_symbol_t *sym;
    
    if(addr >= (addr_t)&kernel_end) {
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
