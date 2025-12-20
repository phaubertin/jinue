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
#include <kernel/domain/services/logging.h>
#include <kernel/domain/services/mman.h>
#include <kernel/domain/services/panic.h>
#include <kernel/infrastructure/i686/drivers/lapic.h>
#include <kernel/infrastructure/i686/cpuinfo.h>
#include <kernel/infrastructure/i686/platform.h>
#include <kernel/interface/i686/asm/idt.h>
#include <kernel/machine/memory.h>
#include <kernel/types.h>
#include <stdbool.h>

/** pointer to start of local APIC memory-mapped register region */
static void *mmio_addr;

/**
 * Get pointer to a memory-mapped 32-bit local APIC register
 * 
 * @param offset register offset
 * @return pointer to register
 */
static inline volatile uint32_t *apic_register(int offset) {
    return (volatile uint32_t *)((addr_t)mmio_addr + offset);
}

/**
 * Read a 32-bit value from a local APIC register
 * 
 * @param offset register offset
 * @return value read from register
 */
static uint32_t read_register(int offset) {
    return *apic_register(offset);
}

/**
 * Write a 32-bit value to a local APIC register
 * 
 * @param offset register offset
 * @param value value to write
 */
static void write_register(int offset, uint32_t value) {
    *apic_register(offset) = value;
}

/**
 * Map the local APIC registers in virtual memory.
 */
static void map_registers(void) {
    paddr_t paddr = platform_get_local_apic_address();

    mmio_addr = map_in_kernel(paddr, APIC_REGS_SIZE, JINUE_PROT_READ | JINUE_PROT_WRITE);

    machine_add_reserved_to_address_map(paddr, APIC_REGS_SIZE);

    /* TODO ensure cacheability attributes are appropriate (MTRRs?) */
}

/**
 * Check the version of the local APIC
 * 
 * Panic if local API version < 16 (0x10).
 */
static void check_version(void) {
    const uint32_t regval = read_register(APIC_REG_VERSION);
    const int version = regval & 0xff;
    const int entries = ((regval >> 16) & 0xff) + 1;

    info(
        "Local APIC version %u (0x%02x) has %u LVT entries",
        version,
        version,
        entries
    );

    if(version < 0x10) {
        panic("Local APIC version 16 (0x10) or above is required.");
    }
}

/**
 * Set the local APIC timer divider
 * 
 * @param divider divider (power of two in range 1..128)
 */
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
            value = 0xb;
            break;
        default:
            error("error: attempting to set local APIC timer divider to: %d", divider);
            panic("Invalid value for local APIC timer divider");
    }

     write_register(APIC_REG_DIVIDE_CONF, value);
}

/**
 * Initialize the local APIC timer
 */
void init_timer(void) {
    set_divider(1);

    write_register(APIC_REG_LVT_TIMER, APIC_LVT_TIMER_PERIODIC | IDT_APIC_TIMER);
    
    /* TODO get frequency from CPUID and/or MSRs and/or calibrate
     *
     * This is QEMU's hardcoded frequency */
    const uint32_t clock_freq_hz = 1000000000;
    const uint32_t initial_count = (clock_freq_hz / TICKS_PER_SECOND) - 1;

    /* Writing the initial count starts the timer. */
    write_register(APIC_REG_INITIAL_COUNT, initial_count);
}

/**
 * Initialize the local APIC, including the local APIC timer
 */
void local_apic_init(void) {
    map_registers();

    check_version();

    /* Setting the mask flag to unmasked/enabled in the spurious vector enables
     * the local APIC. Here, we toggle this flag to reset the local APIC to a
     * known state (i.e. all LVTs masked), and then enable it.
     *
     * The local APIC does not behave in the same way when it has been disabled
     * by software compared to when it is disabled in its reset/power-up state.
     * Notably, if it has been disabled by software, as we do here by toggling
     * the flag, it must be enabled before any other vector can be unmasked.
     * 
     * See section 12.4.7.2 of the Intel 64 and IA-32 Architectures Software
     * Developer’s Manual Volume 3 (3A, 3B, 3C, & 3D): System Programming
     * Guide. */
    write_register(APIC_REG_SPURIOUS_VECT, APIC_SVR_ENABLED);
    write_register(APIC_REG_SPURIOUS_VECT, 0);

    write_register(APIC_REG_SPURIOUS_VECT, APIC_SVR_ENABLED | IDT_APIC_SPURIOUS);

    /* Set task priority class to accept all valid interrupts (priority class > 1). */
    write_register(APIC_REG_TPR, (1 << 4));

    /* Clear pending APIC errors, if any. */
    write_register(APIC_REG_ERROR_STATUS, 0);

    init_timer();
}

/**
 * Signal interrupt servicing completion to local APIC
 */
void local_apic_eoi(void) {
    write_register(APIC_REG_EOI, 0);
}
