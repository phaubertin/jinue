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
#include <kernel/infrastructure/i686/drivers/acpi.h>
#include <kernel/infrastructure/i686/types.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static kern_paddr_t rsdp_paddr = 0;

static acpi_tables_t acpi_tables;

static const acpi_table_def_t table_defs[] = {
    { .signature = "FACP", .ptr = (const void **)&acpi_tables.fadt },
    { .signature = "APIC", .ptr = (const void **)&acpi_tables.madt },
    { .signature = "HPET", .ptr = (const void **)&acpi_tables.hpet },
    { .signature = NULL, .ptr = NULL },
};

/**
 * Validate the ACPI RSDP
 * 
 * At the stage of the boot process where this function is called, the memory
 * where the RSDP is located is mapped 1:1 so a pointer to the RSDP has the
 * same value as its physical address.
 *
 * @param rsdp pointer to ACPI RSDP
 * @return true is RSDP is valid, false otherwise
 */
static bool check_rsdp(const acpi_rsdp_t *rsdp) {
    const char *const signature = "RSD PTR ";

    if(strncmp(rsdp->signature, signature, strlen(signature)) != 0) {
        return false;
    }

    if(!verify_acpi_checksum(rsdp, ACPI_V1_RSDP_SIZE)) {
        return false;
    }

    if(rsdp->revision == ACPI_V1_REVISION) {
        return true;
    }

    if(rsdp->revision != ACPI_V2_REVISION) {
        return false;
    }

    return verify_acpi_checksum(rsdp, sizeof(acpi_rsdp_t));
}

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
        const acpi_rsdp_t *rsdp = (const acpi_rsdp_t *)addr;

        if(check_rsdp(rsdp)) {
            return rsdp;
        }
    }

    const char *bottom  = (const char *)0x10000;
    const char *top     = (const char *)(0xa0000 - 1024);
    const char *ebda    = (const char *)(16 * (*(uint16_t *)ACPI_BDA_EBDA));

    if(ebda < bottom || ebda > top) {
        return NULL;
    }

    for(const char *addr = ebda; addr < ebda + 1024; addr += 16) {
        const acpi_rsdp_t *rsdp = (const acpi_rsdp_t *)addr;

        if(check_rsdp(rsdp)) {
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
