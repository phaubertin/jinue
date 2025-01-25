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

#include <kernel/domain/services/logging.h>
#include <kernel/infrastructure/acpi/acpi.h>
#include <kernel/infrastructure/acpi/tables.h>
#include <kernel/infrastructure/acpi/types.h>
#include <kernel/infrastructure/i686/firmware/acpi.h>
#include <kernel/infrastructure/i686/firmware/bios.h>
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
        .ptr        = (const void **)&acpi_tables.fadt
    },
    {
        .name       = ACPI_MADT_NAME,
        .signature  = ACPI_MADT_SIGNATURE,
        .ptr        = (const void **)&acpi_tables.madt
    },
    {
        .name       = ACPI_HPET_NAME,
        .signature  = ACPI_HPET_SIGNATURE,
        .ptr        = (const void **)&acpi_tables.hpet
    },
    { .signature  = NULL },
};

/**
 * Scan a range of physical memory to find the RSDP
 * 
 * The start and end addresses must both be aligned on a 16-byte boundary.
 *
 * @param from start address of scan
 * @param to end address of scan
 * @return address of RSDP if found, PADDR_NULL otherwise
 */
static uint32_t scan_address_range(uint32_t from, uint32_t to) {
    for(uintptr_t addr = from; addr < to; addr += 16) {
        /* At the stage of the boot process where this function is called, the
         * memory where the floating pointer structure can be located is mapped
         * 1:1 so a pointer to the structure has the same value as its physical
         * address.*/
        const acpi_rsdp_t *rsdp = (const acpi_rsdp_t *)addr;

        if(validate_acpi_rsdp(rsdp)) {
            return addr;
        }
    }

    return PADDR_NULL;
}

/**
 * Scan memory for RSDP
 *
 * @return address of RSDP if found, PADDR_NULL otherwise
 */
static uint32_t scan_for_rsdp(void) {
    uint32_t rsdp = scan_address_range(0x0e0000, 0x100000);

    if(rsdp != PADDR_NULL) {
        return rsdp;
    }

    uint32_t top    = 0xa0000 - KB;
    uint32_t ebda   = get_bios_ebda_addr();

    if(ebda == 0 || ebda > top) {
        return PADDR_NULL;
    }

    return scan_address_range(ebda, ebda + KB);
}

/**
 * Locate the ACPI RSDP in memory
 * 
 * At the stage of the boot process where this function is called, the memory
 * where the ACPI can be located has to be mapped 1:1 so a pointer to the RSDP
 * has the same value as its physical address.
 */
void find_acpi_rsdp(void) {
    rsdp_paddr = scan_for_rsdp();
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

void report_acpi_tables(void) {
    info("ACPI:");

    for(int idx = 0; table_defs[idx].signature != NULL; ++idx) {
        const acpi_table_def_t *def = &table_defs[idx];

        if(*def->ptr != NULL) {
            info("  Found %s table", def->name);
        }
    }
}

/**
 * Get the physical address of the ACPI RSDP
 *
 * @return physical address of RSDP if found, zero otherwise
 */
kern_paddr_t acpi_get_rsdp_paddr(void) {
    return rsdp_paddr;
}
