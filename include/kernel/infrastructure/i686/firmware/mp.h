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

#ifndef JINUE_KERNEL_INFRASTRUCTURE_I686_FIRMWARE_MP_H
#define JINUE_KERNEL_INFRASTRUCTURE_I686_FIRMWARE_MP_H

#include <kernel/infrastructure/i686/firmware/asm/mp.h>
#include <kernel/machine/types.h>
#include <stdint.h>

/* Multiprocessor Specification 1.4 section 4.1 MP Floating Pointer
 * Structure */

typedef struct {
    char        signature[4];
    uint32_t    addr;
    uint8_t     length;
    uint8_t     revision;
    uint8_t     checksum;
    uint8_t     feature1;
    uint8_t     feature2;
    uint8_t     feature_reserved[3];
} mp_ptr_struct_t;

/* Multiprocessor Specification 1.4 section 4.2 MP Configuration Table
 * Header */

typedef struct {
    char        signature[4];
    uint16_t    base_length;
    uint8_t     revision;
    uint8_t     checksum;
    char        oem_id[8];
    char        product_id[12];
    uint32_t    oem_table_addr;
    uint16_t    oem_table_size;
    uint16_t    entry_count;
    uint32_t    lapic_addr;
    uint16_t    ext_table_length;
    uint8_t     ext_table_checksum;
    uint8_t     reserved;
    const char  entries[];
} mp_conf_table_t;

/* Multiprocessor Specification 1.4 section 4.3.1 Processor Entries */

typedef struct {
    uint8_t     entry_type;
    uint8_t     apic_id;
    uint8_t     apic_version;
    uint8_t     cpu_flags;
    uint32_t    cpu_signature;
    uint32_t    feature_flags;
    uint32_t    reserved1;
    uint32_t    reserved2;
} mp_entry_processor_t;

/* Multiprocessor Specification 1.4 section 4.3.2 Bus Entries */

typedef struct {
    uint8_t     entry_type;
    uint8_t     bus_id;
    char        bus_type[6];
} mp_entry_bus_t;

/* Multiprocessor Specification 1.4 section 4.3.3 I/O APIC Entries */

typedef struct {
    uint8_t     entry_type;
    uint8_t     apic_id;
    uint8_t     apic_version;
    uint8_t     flag;
    uint32_t    addr;
} mp_entry_ioapic_t;

/* Multiprocessor Specification 1.4 section 4.3.4 I/O Interrupt Assignment
 * Entries and 4.3.5 Local Interrupt Assignment Entries */

typedef struct {
    uint8_t     entry_type;
    uint8_t     intr_type;
    uint16_t    io_intr_flag;
    uint8_t     source_bus_id;
    uint8_t     source_bus_irq;
    uint8_t     dest_apic_id;
    uint8_t     dest_apic_intn;
} mp_entry_intr_t;

void find_mp(const addr_t first1mb);

void init_mp(void);

paddr_t mp_get_local_apic_addr(void);

#endif
