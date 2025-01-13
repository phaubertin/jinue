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
#include <kernel/domain/services/logging.h>
#include <kernel/infrastructure/i686/isa/abi.h>
#include <kernel/infrastructure/elf.h>
#include <kernel/interface/i686/boot.h>
#include <kernel/machine/debug.h>
#include <inttypes.h>
#include <stddef.h>

void machine_dump_call_stack(void) {
    /* This function is called by the panic handler and one potential reason
     * for a kernel panic is an early boot check that the boot information
     * structure is valid. We can't assume that it is valid here. */
    if(!check_bootinfo(false)) {
        warning("warning: cannot dump call stack because boot information structure is invalid.");
        return;
    }

    const bootinfo_t *bootinfo = get_bootinfo();

    info("Call stack dump:");

    for(addr_t fptr = get_fpointer(); fptr != NULL; fptr = get_caller_fpointer(fptr)) {
        addr_t return_addr = get_ret_addr(fptr);

        if(return_addr == NULL) {
            break;
        }
        
        /* We assume e8 xx xx xx xx for call instruction encoding.
         * TODO can we do better than this? */
        return_addr -= 5;
        
        const Elf32_Ehdr *ehdr = bootinfo->kernel_start;
        const Elf32_Sym *symbol = elf_find_function_symbol_by_address(
                ehdr,
                (Elf32_Addr)return_addr);

        if(symbol == NULL) {
            info("  0x%x (unknown)", return_addr);
            continue;
        }

        const char *name = elf_symbol_name(ehdr, symbol);

        if(name == NULL) {
            name = "[unknown]";
        }

        info(   "  %#p (%s+%" PRIuPTR ")",
                return_addr,
                name,
                return_addr - symbol->st_value);
    }
}
