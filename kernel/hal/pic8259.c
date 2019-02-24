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

#include <hal/io.h>
#include <hal/pic8259.h>


void pic8259_init(int intrvect_base) {
	/* Issue ICW1 to start initialization sequence of both interrupt controllers.
	 * Specify there will be an ICW4 in the sequence. Specify the interrupts are
	 * edge-triggered and that the PICs are in a cascaded configuration by
	 * leaving the relevant flags cleared. */
	outb(PIC8259_MASTER_BASE+0, PIC8259_ICW1_1 | PIC8259_ICW1_IC4);
	iodelay();
	outb(PIC8259_SLAVE_BASE+0, PIC8259_ICW1_1 | PIC8259_ICW1_IC4);
	iodelay();

	/* ICW2: base interrupt vector */
	outb(PIC8259_MASTER_BASE+1, intrvect_base);
	iodelay();
	outb(PIC8259_SLAVE_BASE+1, intrvect_base + 8);
	iodelay();

	/* ICW3: master-slave connections */
	outb(PIC8259_MASTER_BASE+1, (1<<PIC8259_CASCADE_INPUT));
	iodelay();
	outb(PIC8259_SLAVE_BASE+1, PIC8259_CASCADE_INPUT);
	iodelay();

	/* ICW4: Use 8088/8086 mode */
	outb(PIC8259_MASTER_BASE+1, PIC8259_ICW4_UPM);
	iodelay();
	outb(PIC8259_SLAVE_BASE+1, PIC8259_ICW4_UPM);
	iodelay();

	/* Set interrupt mask: all masked */
	outb(PIC8259_MASTER_BASE+1, 0xff & ~(1<<PIC8259_CASCADE_INPUT));
	iodelay();
	outb(PIC8259_SLAVE_BASE+1, 0xff);
	iodelay();
}

void pic8259_mask_irq(int irq) {
	if(irq < PIC8259_IRQ_COUNT) {
		if(irq < 8 && irq != PIC8259_CASCADE_INPUT) {
			int mask = inb(PIC8259_MASTER_BASE+1);
			iodelay();

			mask |= (1<<irq);
			outb(PIC8259_MASTER_BASE+1, mask);
			iodelay();
		}
		else {
			int mask = inb(PIC8259_SLAVE_BASE+1);
			iodelay();

			mask |= (1<<(irq - 8));
			outb(PIC8259_SLAVE_BASE+1, mask);
			iodelay();
		}
	}
}

void pic8259_unmask_irq(int irq) {
	if(irq < PIC8259_IRQ_COUNT) {
		int master_irq;

		if(irq < 8) {
			master_irq = irq;
		}
		else {
			/* Unmask interrupt in slave PIC. */
			int mask = inb(PIC8259_SLAVE_BASE+1);
			iodelay();

			mask &= ~(1<<(irq - 8));
			outb(PIC8259_SLAVE_BASE+1, mask);
			iodelay();

			/* We will also want to unmask the cascaded interrupt line in the
			 * master PIC. */
			master_irq = PIC8259_CASCADE_INPUT;
		}

		if(irq != PIC8259_CASCADE_INPUT) {
			int mask = inb(PIC8259_MASTER_BASE+1);
			iodelay();

			mask &= ~(1<<master_irq);
			outb(PIC8259_MASTER_BASE+1, mask);
			iodelay();
		}
	}
}

void pic8259_eoi(int irq) {
	if(irq < PIC8259_IRQ_COUNT) {
		if(irq >= 8) {
			outb(PIC8259_SLAVE_BASE+0, PIC8259_EOI);
			iodelay();
		}

		outb(PIC8259_MASTER_BASE+0, PIC8259_EOI);
		iodelay();

		pic8259_unmask_irq(irq);
	}
}
