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

#ifndef JINUE_KERNEL_INFRASTRUCTURE_ACPI_TABLES_H
#define JINUE_KERNEL_INFRASTRUCTURE_ACPI_TABLES_H

#include <kernel/infrastructure/acpi/asm/tables.h>
#include <kernel/infrastructure/acpi/types.h>
#include <kernel/infrastructure/compiler.h>
#include <stdint.h>

/* ACPI 6.4 section 5.2.5.3 Root System Description Pointer (RSDP) Structure */

typedef PACKED_STRUCT {
    char        signature[8];
    uint8_t     checksum;
    char        oemid[6];
    uint8_t     revision;
    uint32_t    rsdt_address;
    uint32_t    length;
    uint64_t    xsdt_address;
    uint8_t     extended_checksum;
    uint8_t     reserved[3];
} acpi_rsdp_t;

/* ACPI 6.4 section 5.2.6 System Description Table Header */

typedef struct {
    char        signature[4];
    uint32_t    length;
    uint8_t     revision;
    uint8_t     checksum;
    char        oemid[6];
    char        oem_table_id[8];
    uint32_t    oem_revision;
    uint32_t    creator_id;
    uint32_t    creator_revision;
} acpi_table_header_t;

/* ACPI 6.4 section 5.2.3.2 Generic Address Structure (GAS) */

typedef PACKED_STRUCT {
    uint8_t             addr_space_id;
    uint8_t             reg_width;
    uint8_t             reg_offset;
    uint8_t             access_size;
    uint64_t            address;
} acpi_gas_t;

/* ACPI 6.4 section 5.2.7 Root System Description Table (RSDT) and 5.2.8
 * Extended System Description Table (XSDT) */

typedef PACKED_STRUCT {
    acpi_table_header_t header;
    uint32_t            entries[];
} acpi_rsdt_t;

/* ACPI 6.4 section 5.2.9 Fixed ACPI Description Table (FADT) */

typedef PACKED_STRUCT {
    acpi_table_header_t header;
    uint32_t            firmware_ctrl;
    uint32_t            dsdt;
    uint8_t             reserved1;
    uint8_t             preferred_pm_profile;
    uint16_t            sci_int;
    uint32_t            sci_cmd;
    uint8_t             acpi_enable;
    uint8_t             acpi_disable;
    uint8_t             s4bios_req;
    uint8_t             pstate_cnt;
    uint32_t            pm1a_evt_blk;
    uint32_t            pm1b_evt_blk;
    uint32_t            pm1a_cnt_blk;
    uint32_t            pm1b_cnt_blk;
    uint32_t            pm2_cnt_blk;
    uint32_t            pm_tmr_blk;
    uint32_t            gpe0_blk;
    uint32_t            gpe1_blk;
    uint8_t             pm1_evt_len;
    uint8_t             pm1_cnt_len;
    uint8_t             pm2_cnt_len;
    uint8_t             pm_tmr_len;
    uint8_t             gpe0_blk_len;
    uint8_t             gpe1_blk_len;
    uint8_t             gpe1_base;
    uint8_t             cst_cnt;
    uint16_t            p_lvl2_lat;
    uint16_t            p_lvl3_lat;
    uint16_t            flush_size;
    uint16_t            flush_stride;
    uint8_t             duty_offset;
    uint8_t             duty_width;
    uint8_t             day_alrm;
    uint8_t             mon_alrm;
    uint8_t             century;
    uint16_t            iapc_boot_arch;
    uint8_t             reserved2;
    uint32_t            flags;
    acpi_gas_t          reset_reg;
    uint8_t             reset_value;
    uint16_t            arm_boot_arch;
    uint8_t             fadt_minor_version;
    uint64_t            x_firmware_ctrl;
    uint64_t            x_dsdt;
    acpi_gas_t          x_pm1a_evt_blk;
    acpi_gas_t          x_pm1b_evt_blk;
    acpi_gas_t          x_pm1a_cnt_blk;
    acpi_gas_t          x_pm1b_cnt_blk;
    acpi_gas_t          x_pm2_cnt_blk;
    acpi_gas_t          x_pm_tmr_blk;
    acpi_gas_t          x_gpe0_blk;
    acpi_gas_t          x_gpe1_blk;
    acpi_gas_t          sleep_control_reg;
    acpi_gas_t          sleep_status_reg;
    uint64_t            hypervisor_vendor_identity;
} acpi_fadt_t;

/* ACPI 6.4 section 5.2.12 Multiple APIC Description Table (MADT) */

typedef PACKED_STRUCT {
    acpi_table_header_t header;
    uint32_t            local_intr_controller_addr;
    uint32_t            flags;
    const char          entries[];
} acpi_madt_t;

typedef PACKED_STRUCT {
    uint8_t             type;
    uint8_t             length;
} madt_entry_header_t;

/* ACPI 6.4 section 5.2.12.2 Processor Local APIC Structure */

typedef PACKED_STRUCT {
    madt_entry_header_t header;
    uint8_t             processor_uid;
    uint8_t             apic_id;
    uint32_t            flags;
} acpi_madt_lapic_t;

/* ACPI 6.4 section 5.2.12.3 I/O APIC Structure */

typedef PACKED_STRUCT {
    madt_entry_header_t header;
    uint8_t             apic_id;
    uint8_t             reserved;
    uint32_t            addr;
    uint32_t            intr_base;
} acpi_madt_ioapic_t;

/* ACPI 6.4 section 5.2.12.5 Interrupt Source Override Structure */

typedef PACKED_STRUCT {
    madt_entry_header_t header;
    uint8_t             bus;
    uint8_t             source;
    uint32_t            global_sys_interrupt;
    uint16_t            flags;
} acpi_madt_src_override_t;

/* ACPI 6.4 section 5.2.12.6 Non-Maskable Interrupt (NMI) Source Structure */

typedef PACKED_STRUCT {
    madt_entry_header_t header;
    uint16_t            flags;
    uint32_t            global_sys_interrupt;
} acpi_madt_nmi_source_t;

/* ACPI 6.4 section 5.2.12.7 Local APIC NMI Structure */

typedef PACKED_STRUCT {
    madt_entry_header_t header;
    uint8_t             processor_uid;
    uint16_t            flags;
    uint8_t             lint_num;
} acpi_madt_lapic_nmi_t;

/* ACPI 6.4 section 5.2.12.8 Local APIC Address Override Structure */

typedef PACKED_STRUCT {
    madt_entry_header_t header;
    uint16_t            reserved;
    uint64_t            lapic_addr;
} acpi_madt_lapic_addr_t;

/* ACPI 6.4 section 5.2.12.12 Processor Local x2APIC Structure */

typedef PACKED_STRUCT {
    madt_entry_header_t header;
    uint16_t            reserved;
    uint32_t            apic_id;
    uint32_t            flags;
    uint32_t            processor_uid;
} acpi_madt_x2apic_t;

/* ACPI 6.4 section 5.2.12.13. Local x2APIC NMI Structure */

typedef PACKED_STRUCT {
    madt_entry_header_t header;
    uint16_t            flags;
    uint32_t            processor_uid;
    uint8_t             lint_num;
    uint8_t             reserved[3];
} acpi_madt_x2apic_nmi_t;

/* ACPI 6.4 section 5.2.12.19 Multiprocessor Wakeup Structure */

typedef PACKED_STRUCT {
    madt_entry_header_t header;
    uint16_t            mailbox_version;
    uint32_t            reserved;
    uint64_t            mailbox_addr;
} acpi_madt_wakeup_t;

/* IA-PC HPET (High Precision Event Timers) Specification
 * Section 3.2.4 "The ACPI 2.0 HPET Description Table (HPET)"
 * 
 * https://www.intel.com/content/dam/www/public/us/en/documents/technical-specifications/software-developers-hpet-spec-1-0a.pdf
 */

typedef PACKED_STRUCT {
    acpi_table_header_t header;
    uint32_t            event_timer_block_id;
    acpi_gas_t          base_address;
    uint8_t             hpet_number;
    uint16_t            periodic_min_tick;
    uint8_t             prot_and_oem;
} acpi_hpet_t;

#endif
