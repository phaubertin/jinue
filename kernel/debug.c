/*
 * Copyright (C) 2019 Philippe Aubertin.
 * All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the author nor the names of other contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <jinue/shared/types.h>
#include <hal/abi.h>
#include <hal/boot.h>
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
        int retval = elf_find_symbol_by_address_and_type(
                &symbol,
                boot_info->kernel_start,
                (Elf32_Addr)return_addr,
                STT_FUNCTION);

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
