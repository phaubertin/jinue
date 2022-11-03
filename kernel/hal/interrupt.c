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

#include <kernel/hal/interrupt.h>
#include <kernel/hal/io.h>
#include <kernel/hal/pic8259.h>
#include <kernel/hal/x86.h>
#include <kernel/logging.h>
#include <kernel/panic.h>
#include <kernel/syscall.h>
#include <kernel/types.h>
#include <inttypes.h>


void dispatch_interrupt(trapframe_t *trapframe) {
    unsigned int    ivt         = trapframe->ivt;
    uintptr_t       eip         = trapframe->eip;
    uint32_t        errcode     = trapframe->errcode;
    
    /* exceptions */
    if(ivt <= IDT_LAST_EXCEPTION) {
        info(
                "EXCEPT: %u cr2=%#" PRIx32 " errcode=%#" PRIx32 " eip=%#" PRIxPTR,
                ivt,
                get_cr2(),
                errcode,
                eip);
        
        /* never returns */
        panic("caught exception");
    }
    
    if(ivt == JINUE_SYSCALL_IRQ) {
    	/* interrupt-based system call implementation */
        dispatch_syscall(trapframe);
    }
    else if(ivt >= IDT_PIC8259_BASE && ivt < IDT_PIC8259_BASE + PIC8259_IRQ_COUNT) {
    	int irq = ivt - IDT_PIC8259_BASE;
        info("IRQ: %i (vector %u)", irq, ivt);
        pic8259_ack(irq);
    }
    else {
    	info("INTR: vector %u", ivt);
    }
}
