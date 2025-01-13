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

#include <jinue/shared/asm/mman.h>
#include <kernel/application/asm/ticks.h>
#include <kernel/domain/services/mman.h>
#include <kernel/infrastructure/i686/drivers/lapic.h>
#include <kernel/infrastructure/i686/cpuinfo.h>
#include <kernel/interface/i686/asm/idt.h>
#include <kernel/types.h>
#include <stdbool.h>

static void *mmio_addr;

static void set_register(int offset, uint32_t value) {
    uint32_t *reg = (uint32_t *)((addr_t)mmio_addr + offset);
    *reg = value;
}

static void set_divider(int divider) {
    uint32_t value;

    switch(divider) {
        case 128:
            value = 0xa;
            break;
        case 64:
            value = 9;
            break;
        case 32:
            value = 8;
            break;
        case 16:
            value = 3;
            break;
        case 8:
            value = 2;
            break;
        case 4:
            value = 1;
            break;
        case 2:
            value = 0;
            break;
        case 1:
        default:
            value = 0xb;
    }

     set_register(APIC_REG_DIVIDE_CONF, value);
}

void local_apic_init(void) {
    if(!cpu_has_feature(CPUINFO_FEATURE_LOCAL_APIC)) {
        return;
    }

    // TODO figure out address from firmware table or MSR
    // TODO some APICs are controlled through MSRs instead of MMIO?
    // TODO ensure caheability attributes are appropriate (MTRRs?)
    mmio_addr = map_in_kernel(0xfee00000, 4096, JINUE_PROT_READ | JINUE_PROT_WRITE);
    
    /* TODO get frequency from CPUID and/or MSRs and/or calibrate
     *
     * This is QEMU's hardcoded frequency */
    const uint32_t clock_freq_hz = 1000000000;
    const uint32_t initial_count = (clock_freq_hz / TICKS_PER_SECOND) - 1;

    /* Configure and start timer. */
    set_divider(1);
    set_register(APIC_REG_INITIAL_COUNT, initial_count);

    set_register(APIC_REG_LVT_TIMER, APIC_LVT_TIMER_PERIODIC | IDT_APIC_TIMER);
    set_register(APIC_REG_SPURIOUS_VECT, APIC_SVR_ENABLED | IDT_APIC_SPURIOUS);
}

void local_apic_eoi(void) {
    set_register(APIC_REG_EOI, 0);
}
