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
#include <kernel/infrastructure/i686/types.h>
#include <kernel/utils/asm/utils.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static kern_paddr_t rsdp_paddr = 0;

static struct {
    const acpi_fadt_t *fadt;
    const acpi_madt_t *madt;
    const acpi_hpet_t *hpet;
} acpi_tables;

static const acpi_table_def_t table_defs[] = {
    { .signature = ACPI_FADT_SIGNATURE, .ptr = (const void **)&acpi_tables.fadt },
    { .signature = ACPI_MADT_SIGNATURE, .ptr = (const void **)&acpi_tables.madt },
    { .signature = ACPI_HPET_SIGNATURE, .ptr = (const void **)&acpi_tables.hpet },
    { .signature = NULL, .ptr = NULL },
};

/**
 * Find the RSDP in memory
 * 
 * At the stage of the boot process where this function is called, the memory
 * where the RSDP is located is mapped 1:1 so a pointer to the RSDP has the
 * same value as its physical address.
 *
 * @return pointer to RSDP if found, NULL otherwise
 */
static const acpi_rsdp_t *find_rsdp(void) {
    const char *const start = (const char *)0x0e0000;
    const char *const end   = (const char *)0x100000;

    for(const char *addr = start; addr < end; addr += 16) {
        /* At the stage of the boot process where this function is called, the
         * memory where the RSDP is located is mapped 1:1 so a pointer to the
         * RSDP has the same value as its physical address. */
        const acpi_rsdp_t *rsdp = (const acpi_rsdp_t *)addr;

        if(verify_acpi_rsdp(rsdp)) {
            return rsdp;
        }
    }

    const char *top     = (const char *)(0xa0000 - KB);
    const char *ebda    = (const char *)get_bios_ebda_addr();

    if(ebda == NULL || ebda > top) {
        return NULL;
    }

    for(const char *addr = ebda; addr < ebda + KB; addr += 16) {
        const acpi_rsdp_t *rsdp = (const acpi_rsdp_t *)addr;

        if(verify_acpi_rsdp(rsdp)) {
            return rsdp;
        }
    }

    return NULL;
}

/**
 * Locate the ACPI RSDP in memory
 */
void find_acpi_rsdp(void) {
    /* At the stage of the boot process where this function is called, the memory
     * where the RSDP is located is mapped 1:1 so a pointer to the RSDP has the
     * same value as its physical address. */
    rsdp_paddr = (kern_paddr_t)find_rsdp();
}

/**
 * Initialize ACPI
 */
void init_acpi(void) {
    memset(&acpi_tables, 0, sizeof(acpi_tables));

    if(rsdp_paddr == 0) {
        return;
    }

    map_acpi_tables(rsdp_paddr, table_defs);
}

/**
 * Get the physical address of the ACPI RSDP
 *
 * @return physical address of RSDP if found, zero otherwise
 */
kern_paddr_t acpi_get_rsdp_paddr(void) {
    return rsdp_paddr;
}
