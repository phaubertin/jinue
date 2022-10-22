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

#include <hal/asm/irq.h>
#include <hal/io.h>
#include <hal/pic8259.h>
#include <stdbool.h>

typedef struct {
    bool is_main;
    int io_base;
    int irq_base;
    int mask;
} pic8259_t;

static pic8259_t main_pic8259 = {
    .is_main    = true,
    .io_base    = PIC8259_MAIN_IO_BASE,
    .irq_base   = IDT_PIC8259_BASE,
    .mask       = 0xff & ~(1<<PIC8259_CASCADE_INPUT)
};

static pic8259_t proxied_pic8259 = {
    .is_main    = false,
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
    if(pic8259->is_main) {
        value = 1 << PIC8259_CASCADE_INPUT;
    } else {
        value = PIC8259_CASCADE_INPUT;
    }

    outb(pic8259->io_base + 1, value);
    iodelay();

    /* ICW4: Use 8088/8086 mode */
    outb(pic8259->io_base + 1, PIC8259_ICW4_UPM);
    iodelay();

    /* Set interrupt mask */
    outb(pic8259->io_base + 1, pic8259->mask);
    iodelay();
}

static void ack_eoi(pic8259_t *pic8259) {
    outb(pic8259->io_base + 0, PIC8259_EOI);
    iodelay();
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

void pic8259_ack(int irq) {
    if(irq >= 8) {
        ack_eoi(&proxied_pic8259);
    }

    ack_eoi(&main_pic8259);
    pic8259_unmask(irq);
}
