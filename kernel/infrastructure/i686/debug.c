/*
 * Copyright (C) 2019-2026 Philippe Aubertin.
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
#include <kernel/interface/i686/bootinfo.h>
#include <kernel/interface/i686/trap.h>
#include <kernel/machine/debug.h>
#include <inttypes.h>
#include <stddef.h>
#include <string.h>

/** Print information about a code address
 * 
 * Prints the address, the name of the function and the offset from the start
 * of the function, e.g.
 * 
 *  0xc000cfd6 (handle_trap+22)
 *
 * If the address is not recognized, the output looks like this:
 * 
 *  0x00001234 (unknown)
 * 
 * @param addr the address
 * @return the function name, or NULL if not found
 */
static const char *print_symbol(addr_t addr) {
    const bootinfo_t *bootinfo  = get_bootinfo();
    const Elf32_Ehdr *ehdr      = bootinfo->kernel_start;
    
    const Elf32_Sym *symbol = elf_find_function_symbol_by_address(
            ehdr,
            (Elf32_Addr)addr);

    if(symbol == NULL) {
        info("  0x%x (unknown)", addr);
        return NULL;
    }

    const char *name = elf_symbol_name(ehdr, symbol);

    info("  %#p (%s+%" PRIuPTR ")",
            addr,
            name != NULL ? name: "[unknown]",
            addr - symbol->st_value);
    
    return name;
}

/** Dump the call stack
 * 
 * Example output:
 * 
 *  Call stack dump:
 *    0xc0005aea (panic+106)
 *    0xc000cfb4 (handle_interrupt+260)
 *    0xc000cfd6 (handle_trap+22)
 *    0xc000137a (interrupt_entry+58)
 *    -
 *    0xc000227c (send+12)
 *    0xc000d0e3 (handle_syscall+227)
 *    0xc000cfec (handle_trap+44)
 *    0xc00013cc (fast_intel_entry+52)
 */
void machine_dump_call_stack(void) {
    /* This function is called by the panic handler and one potential reason
     * for a kernel panic is an early boot check that the boot information
     * structure is valid. We can't assume that it is valid here. */
    if(!check_bootinfo(false)) {
        warn(WARNING "cannot dump call stack because boot information structure is invalid.");
        return;
    }

    info("Call stack dump:");

    addr_t frameptr = get_frameptr();

    while(frameptr != NULL) {
        addr_t return_addr = get_return_addr(frameptr);

        if(return_addr == NULL) {
            break;
        }
        
        /* We assume e8 xx xx xx xx for call instruction encoding.
         * TODO can we do better than this? */
        return_addr -= 5;
        
        const char *name = print_symbol(return_addr);
        
        addr_t next = get_caller_frameptr(frameptr);

        if(next != NULL) {
            frameptr = next;
            continue;
        }

        if(name == NULL) {
            break;
        }

        /* A NULL value for the frame pointer indicates the end of the call
         * stack. However, if we are currently handling an interrupt which
         * occurred in the kernel, we want to continue the dump accross the
         * interrupt. */
        if(strcmp(name, "interrupt_entry") != 0) {
            break;
        }

        /* frameptr, which hasn't been updated yet, is the frame pointer that
         * points to handle_trap()'s stack frame.  It is it's return address,
         * so an address in its caller, that has been mapped to the name
         * "interrupt_entry" above.
         * 
         * The first argument to handle_trap() is the trap frame. */
        const trapframe_t *trapframe = get_first_pointer_arg(frameptr);

        if(!is_trap_from_kernel(trapframe)) {
            break;
        }

        info("  -");
        (void)print_symbol((void *)trapframe->eip);

        frameptr = (void *)trapframe->ebp;
    }
}
