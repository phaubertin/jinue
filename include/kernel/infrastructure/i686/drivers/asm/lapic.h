/*
 * Copyright (C) 2025 Philippe Aubertin.
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

#ifndef JINUE_KERNEL_INFRASTRUCTURE_I686_DRIVERS_ASM_LAPIC_H
#define JINUE_KERNEL_INFRASTRUCTURE_I686_DRIVERS_ASM_LAPIC_H

#define APIC_REG_ID             0x20

#define APIC_REG_VERSION        0x30

#define APIC_REG_TPR            0x80

#define APIC_REG_APR            0x90

#define APIC_REG_PPR            0xa0

#define APIC_REG_EOI            0xb0

#define APIC_REG_RRD            0xc0

#define APIC_REG_LOGICAL_DEST   0xd0

#define APIC_REG_DEST_FORMAT    0xe0

#define APIC_REG_SPURIOUS_VECT  0xf0

#define APIC_REG_ISR            0x100

#define APIC_REG_TMR            0x180

#define APIC_REG_IRR            0x200

#define APIC_REG_ERROR_STATUS   0x280

#define APIC_REG_LVT_CMCI       0x2f0

#define APIC_REG_LVT_TIMER      0x320

#define APIC_REG_LVT_THERMAL    0x330

#define APIC_REG_LVT_PERF_MON   0x340

#define APIC_REG_LVT_LINT0      0x350

#define APIC_REG_LVT_LINT1      0x360

#define APIC_REG_LVT_ERROR      0x370

#define APIC_REG_INITIAL_COUNT  0x380

#define APIC_REG_CURRENT_COUNT  0x390

#define APIC_REG_DIVIDE_CONF    0x3e0


#define APIC_LVT_DELIVERY_FIXED     0

#define APIC_LVT_DELIVERY_SMI       0x200

#define APIC_LVT_DELIVERY_NMI       0x400

#define APIC_LVT_DELIVERY_INIT      0x600

#define APIC_LVT_DELIVERY_EXTINT    0x700


#define APIC_LVT_PENDING            (1 << 12)

#define APIC_LVT_POLARITY_HIGH      0

#define APIC_LVT_POLARITY_LOW       (1 << 13)

#define APIC_LVT_TRIGGER_EDGE       0

#define APIC_LVT_TRIGGER_LEVEL      (1 << 15)

#define APIC_LVT_MASKED             (1 << 16)

#define APIC_LVT_TIMER_ONE_SHOT     0

#define APIC_LVT_TIMER_PERIODIC     (1 << 17)

#define APIC_LVT_TIMER_TSC_DEADLINE (2 << 17)


#define APIC_SVR_DISABLED           0

#define APIC_SVR_ENABLED            (1 << 8)

#endif
