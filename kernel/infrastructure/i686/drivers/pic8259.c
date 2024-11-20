/*
 * Copyright (C) 2019-2024 Philippe Aubertin.
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

#include <kernel/infrastructure/i686/drivers/pic8259.h>
#include <kernel/infrastructure/i686/drivers/iodelay.h>
#include <kernel/infrastructure/i686/isa/io.h>
#include <kernel/interface/i686/asm/idt.h>
#include <stdbool.h>

typedef struct {
    bool is_proxied;
    int io_base;
    int irq_base;
    int mask;
} pic8259_t;

static pic8259_t main_pic8259 = {
    .is_proxied = false,
    .io_base    = PIC8259_MAIN_IO_BASE,
    .irq_base   = IDT_PIC8259_BASE,
    .mask       = 0xff & ~(1<<PIC8259_CASCADE_INPUT)
};

static pic8259_t proxied_pic8259 = {
    .is_proxied = true,
    .io_base    = PIC8259_PROXIED_IO_BASE,
    .irq_base   = IDT_PIC8259_BASE + 8,
    .mask       = 0xff
};

static void initialize(const pic8259_t *pic8259) {
    int value;

    /* Issue ICW1 to start initialization sequence. Specify the interrupts are
     * edge-triggered and that the PICs are in a cascaded configuration by
     * leaving the relevant flags cleared. */
    outb(pic8259->io_base + 0, PIC8259_ICW1_1 | PIC8259_ICW1_IC4);
    iodelay();

    /* ICW2: base interrupt vector */
    outb(pic8259->io_base + 1, pic8259->irq_base);
    iodelay();

    /* ICW3: cascading connections */
    if(pic8259->is_proxied) {
        value = PIC8259_CASCADE_INPUT;
    } else {
        value = 1 << PIC8259_CASCADE_INPUT;
    }

    outb(pic8259->io_base + 1, value);
    iodelay();

    /* ICW4: Use 8088/8086 mode */
    value = PIC8259_ICW4_UPM;

    if(!pic8259->is_proxied) {
        /* special fully nested mode for main */
        value |= PIC8259_ICW4_SFNM;
    }

    outb(pic8259->io_base + 1, value);
    iodelay();

    /* Set interrupt mask */
    outb(pic8259->io_base + 1, pic8259->mask);
    iodelay();

    /* We are only ever going to read the ISR, never the IRR, so let's
     * enable this once here and not have to do it for each read. */
    outb(pic8259->io_base + 0, PIC8259_OCW3_READ_ISR);
    iodelay();
}

static void eoi(pic8259_t *pic8259) {
    outb(pic8259->io_base + 0, PIC8259_OCW2_EOI);
    iodelay();
}

static uint8_t read_isr(pic8259_t *pic8259) {
    return inb(pic8259->io_base + 0);
}

void pic8259_init() {
    initialize(&main_pic8259);
    initialize(&proxied_pic8259);
}

static void mask_irqs(pic8259_t *pic8259, int mask) {
    pic8259->mask |= mask;
    outb(pic8259->io_base + 1, pic8259->mask);
    iodelay();
}

static void unmask_irqs(pic8259_t *pic8259, int mask) {
    pic8259->mask &= ~mask;
    outb(pic8259->io_base + 1, pic8259->mask);
    iodelay();
}

void pic8259_mask(int irq) {
    if(irq < 8) {
        if(irq != PIC8259_CASCADE_INPUT) {
            mask_irqs(&main_pic8259, 1 << irq);
        }
    }
    else {
        mask_irqs(&proxied_pic8259, 1 << (irq - 8));
    }
}

void pic8259_unmask(int irq) {
    if(irq < 8) {
        unmask_irqs(&main_pic8259, 1 << irq);
    }
    else {
        unmask_irqs(&proxied_pic8259, 1 << (irq - 8));
    }
}

void pic8259_eoi(int irq) {
    if(irq >= 8) {
        eoi(&proxied_pic8259);
        iodelay();

        /* Special fully nested mode: do not send EIO to main controller if
         * interrupts are still being service on the proxied one. */
        uint8_t isr = read_isr(&proxied_pic8259);

        if(isr != 0) {
            return;
        }
    }

    eoi(&main_pic8259);
}

bool pic8259_is_spurious(int irq) {
    if(irq != 7 && irq != 15) {
        return false;
    }

    const uint8_t mask = (1 << 7);

    if(irq == 7) {
        /* If we got interrupted for IRQ 7 but IRQ 7 isn't actually being
         * serviced by the main PIC, then this is a spurious interrupt.
         *
         * Don't send a EOI either way:
         * - In the case of a spurious interrupt, no IRQ 7 is in service, so no
         *   EOI should be sent.
         * - In the case of an actual interrupt, the handler will handle it as
         *   any other hardware interrupt and will call pic8259_eoi() later. */
        uint8_t isr = read_isr(&main_pic8259);
        return (isr & mask) == 0;
    }

    uint8_t isr = read_isr(&proxied_pic8259);

    if((isr & mask) != 0) {
        return false;
    }

    /* Spurious interrupt on the proxied PIC: we must not send a EOI to the
     * proxied PIC, but we must send it to the main PIC that got interrupted
     * by the proxied PIC.
     * 
     * This is true unless another interrupt is in service on the proxied PIC
     * (special fully nested mode). */
    if(isr == 0) {
        eoi(&main_pic8259);
    }

    return true;
}
