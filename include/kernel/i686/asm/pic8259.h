/*
 * Copyright (C) 2019-2022 Philippe Aubertin.
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

#ifndef JINUE_KERNEL_I686_ASM_PIC8259_H
#define JINUE_KERNEL_I686_ASM_PIC8259_H

/** Base I/O port for the main interrupt controller */
#define PIC8259_MAIN_IO_BASE    0x20

/** Base I/O port for the proxied interrupt controller */
#define PIC8259_PROXIED_IO_BASE 0xa0

/** Proxied PIC is connected to input 2 of the main PIC. */
#define PIC8259_CASCADE_INPUT   2

/** Number of IRQs handled by both cascaded PIC8259s together */
#define PIC8259_IRQ_COUNT       16

/** ICW1 bit 0: ICW4 needed */
#define PIC8259_ICW1_IC4        (1<<0)

/** ICW1 bit 1: single (1) or cascade (0) mode */
#define PIC8259_ICW1_SNGL       (1<<1)

/** ICW1 bit 3: level-triggered (1) or edge-triggered (0) interrupts  */
#define PIC8259_ICW1_LTIM       (1<<3)

/** ICW1 bit 4: a control word with this bit set indicates this is ICW1 */
#define PIC8259_ICW1_1          (1<<4)

/** ICW4 bit 0: 8086/8088 mode (1) or MCS-80/85 mode (0) */
#define PIC8259_ICW4_UPM        (1<<0)

/** ICW4 bit 1: Auto EOI*/
#define PIC8259_ICW4_AEOI       (1<<1)

/** OCW2: non-specific EOI command */
#define PIC8259_EOI             0x20

#endif
