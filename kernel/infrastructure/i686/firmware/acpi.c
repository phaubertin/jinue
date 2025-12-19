/*
 * Copyright (C) 2024-2025 Philippe Aubertin.
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

#include <kernel/infrastructure/acpi/acpi.h>
#include <kernel/infrastructure/acpi/tables.h>
#include <kernel/infrastructure/acpi/types.h>
#include <kernel/infrastructure/i686/firmware/acpi.h>
#include <kernel/infrastructure/i686/firmware/bios.h>
#include <kernel/infrastructure/i686/platform.h>
#include <kernel/infrastructure/i686/types.h>
#include <kernel/utils/asm/utils.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define PADDR_NULL 0

static uint32_t rsdp_paddr = 0;

static struct {
    const acpi_fadt_t *fadt;
    const acpi_madt_t *madt;
    const acpi_hpet_t *hpet;
} acpi_tables;

static const acpi_table_def_t table_defs[] = {
    {
        .name       = ACPI_FADT_NAME,
        .signature  = ACPI_FADT_SIGNATURE,
        .size       = sizeof(acpi_fadt_t),
        .ptr        = (const void **)&acpi_tables.fadt
    },
    {
        .name       = ACPI_MADT_NAME,
        .signature  = ACPI_MADT_SIGNATURE,
        .size       = sizeof(acpi_madt_t),
        .ptr        = (const void **)&acpi_tables.madt
    },
    {
        .name       = ACPI_HPET_NAME,
        .signature  = ACPI_HPET_SIGNATURE,
        .size       = sizeof(acpi_hpet_t),
        .ptr        = (const void **)&acpi_tables.hpet
    },
    { .signature  = NULL },
};

/**
 * Scan a range of physical memory to find the RSDP
 * 
 * The start and end addresses must both be aligned on a 16-byte boundary.
 *
 * @param first1mb mapping of first 1MB of memory
 * @param from start address of scan
 * @param to end address of scan
 * @return address of RSDP if found, PADDR_NULL otherwise
 */
static uint32_t scan_address_range(const addr_t first1mb, uint32_t from, uint32_t to) {
    for(uintptr_t addr = from; addr < to; addr += 16) {
        /* At the stage of the boot process where this function is called, the
         * memory where the floating pointer structure can be located is mapped
         * 1:1 so a pointer to the structure has the same value as its physical
         * address.*/
        const acpi_rsdp_t *rsdp = (const acpi_rsdp_t *)&first1mb[addr];

        if(validate_acpi_rsdp(rsdp)) {
            return addr;
        }
    }

    return PADDR_NULL;
}

/**
 * Scan memory for RSDP
 * 
 * The ranges where the RSDP can be located are defined in section 5.2.5.1 of
 * the ACPI Specification:
 * 
 * " OSPM finds the Root System Description Pointer (RSDP) structure by
 *   searching physical memory ranges on 16-byte boundaries for a valid Root
 *   System Description Pointer structure signature and checksum match as
 *   follows:
 *     * The first 1 KB of the Extended BIOS Data Area (EBDA). For EISA or MCA
 *       systems, the EBDA can be found in the two-byte location 40:0Eh on the
 *       BIOS data area.
 *     * The BIOS read-only memory space between 0E0000h and 0FFFFFh. "
 *
 * @param first1mb mapping of first 1MB of memory
 * @return physical address of RSDP if found, PADDR_NULL otherwise
 */
static uint32_t scan_for_rsdp(const addr_t first1mb) {
    uint32_t ebda = get_bios_ebda_addr(first1mb);

    if(ebda != 0 && ebda <= 0xa0000 - KB) {
        uint32_t rsdp = scan_address_range(first1mb, ebda, ebda + KB);
        
        if(rsdp != PADDR_NULL) {
            return rsdp;
        }
    }

    return scan_address_range(first1mb, 0x0e0000, 0x100000);
}

/**
 * Locate the ACPI RSDP in memory
 * 
 * @param first1mb mapping of first 1MB of memory
 */
void find_acpi_rsdp(const addr_t first1mb) {
    rsdp_paddr = scan_for_rsdp(first1mb);
}

/**
 * Initialize ACPI
 */
void init_acpi(void) {
    memset(&acpi_tables, 0, sizeof(acpi_tables));

    if(rsdp_paddr == PADDR_NULL) {
        return;
    }

    map_acpi_tables(rsdp_paddr, table_defs);
}

/**
 * Log information regarding ACPI
 */
void report_acpi(void) {
    report_acpi_tables(table_defs);
}

/**
 * Get the physical address of the ACPI RSDP
 *
 * @return physical address of RSDP if found, zero otherwise
 */
uint32_t acpi_get_rsdp_paddr(void) {
    return rsdp_paddr;
}

/**
 * Detect presence of VGA
 * 
 * @return true if present, false otherwise
 */
bool acpi_is_vga_present(void) {
    if(acpi_tables.fadt == NULL) {
        return true;
    }

    /* From the description if bit 2 "VGA Not Present" of IAPC_BOOT_ARCH in Table
     * 5.11 of the ACPI Specification:
     * 
     * " If set, indicates to OSPM that it must not blindly probe the VGA hardware
     *   (that responds to MMIO addresses A0000h-BFFFFh and IO ports 3B0h-3BBh and
     *   3C0h-3DFh) that may cause machine check on this system. If clear,
     *   indicates to OSPM that it is safe to probe the VGA hardware. "
     */
    return !(acpi_tables.fadt->iapc_boot_arch & ACPI_IAPC_BOOT_ARCH_VGA_NOT_PRESENT);
}

/**
 * Determine the physical address of each CPU's local APIC
 * 
 * @return address of local APIC, PLATFORM_UNKNOWN_LOCAL_APIC_ADDR if unknown
 */
paddr_t acpi_get_local_apic_address(void) {
    if(acpi_tables.madt == NULL) {
        return UNKNOWN_LOCAL_APIC_ADDR;
    }

    /* TODO use 64-bit value in Local APIC Address Override Structure if present
     * 
     * See section 5.2.12.8 of the ACPI specification. */

    return acpi_tables.madt->local_intr_controller_addr;
}
