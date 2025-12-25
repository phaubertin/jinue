/*
 * Copyright (C) 2019-2025 Philippe Aubertin.
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

#include <kernel/application/interrupts.h>
#include <kernel/domain/services/logging.h>
#include <kernel/domain/services/panic.h>
#include <kernel/infrastructure/i686/drivers/lapic.h>
#include <kernel/infrastructure/i686/drivers/pic8259.h>
#include <kernel/infrastructure/i686/isa/regs.h>
#include <kernel/interface/i686/asm/exceptions.h>
#include <kernel/interface/i686/asm/idt.h>
#include <kernel/interface/i686/asm/irq.h>
#include <kernel/interface/i686/interrupts.h>
#include <inttypes.h>


static void handle_exception(unsigned int ivt, uintptr_t eip, uint32_t errcode) {
    info(   "EXCEPT: %u cr2=%#" PRIx32 " errcode=%#" PRIx32 " eip=%#" PRIxPTR,
            ivt,
            get_cr2(),
            errcode,
            eip);
    
    panic("caught exception");
}

static void handle_pic8259_interrupt(unsigned int irq) {
    if(pic8259_is_spurious(irq)) {
        spurious_interrupt();
        return;
    }

    /* For all hardware interrupts, we mask the interrupt and let the driver
     * handling it unmask it when it's done. This prevents the kernel from
     * being interrupted repeatedly by level-triggered interrupts in the
     * meantime. */
    pic8259_mask(irq);

    hardware_interrupt(irq);
    
    pic8259_eoi(irq);
}

static void handle_unexpected_interrupt(unsigned int ivt) {
    info("INTR: vector %u", ivt);
}

void handle_interrupt(trapframe_t *trapframe) {
    unsigned int ivt = trapframe->ivt;

    if(ivt <= IDT_LAST_EXCEPTION) {
        handle_exception(ivt, trapframe->eip, trapframe->errcode);
    } else if(ivt == IDT_APIC_TIMER) {
        tick_interrupt();
        local_apic_eoi();
    } else if(ivt == IDT_APIC_SPURIOUS) {
        spurious_interrupt();
    } else if(ivt >= IDT_PIC8259_BASE && ivt < IDT_PIC8259_BASE + PIC8259_IRQ_COUNT) {
        handle_pic8259_interrupt(ivt - IDT_PIC8259_BASE);
    } else {
        handle_unexpected_interrupt(ivt);
    }
}
