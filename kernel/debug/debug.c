#include <jinue-common/types.h>
#include <hal/boot.h>
#include <hal/frame_pointer.h>
#include <hal/kernel.h>
#include <elf.h>
#include <stddef.h>
#include <debug.h>
#include <printk.h>


void dump_call_stack(void) {
    addr_t               fptr;

    const boot_info_t *boot_info = get_boot_info();

    printk("Call stack dump:\n");

    fptr = get_fpointer();
    
    while(fptr != NULL) {
        addr_t return_addr = get_ret_addr(fptr);
        if(return_addr == NULL) {
            break;
        }
        
        /* assume e8 xx xx xx xx for call instruction encoding */
        return_addr -= 5;
        
        elf_symbol_t symbol;
        int retval = elf_lookup_symbol(
                boot_info->kernel_start,
                (Elf32_Addr)return_addr,
                STT_FUNCTION,
                &symbol);

        if(retval < 0) {
            printk("\t0x%x (unknown)\n", return_addr);
        }
        else {
            const char *name = symbol.name;

            if(name == NULL) {
                name = "[unknown]";
            }

            printk(
                    "\t0x%x (%s+%u)\n",
                    return_addr,
                    name,
                    return_addr - symbol.addr);
        }
        
        fptr = get_caller_fpointer(fptr);
    }
}
