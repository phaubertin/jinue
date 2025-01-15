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

#include <jinue/shared/asm/mman.h>
#include <kernel/domain/services/logging.h>
#include <kernel/domain/services/mman.h>
#include <kernel/infrastructure/i686/drivers/asm/vga.h>
#include <kernel/infrastructure/i686/drivers/acpi.h>
#include <kernel/infrastructure/acpi/types.h>
#include <kernel/infrastructure/i686/types.h>
#include <kernel/machine/acpi.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static kern_paddr_t rsdp_paddr = 0;

static acpi_tables_t tables;

/**
 * Verify the checksum of an ACPI data structure
 *
 * @param buffer pointer to ACPI data structure
 * @param buflen size of ACPI data structure
 * @return true for correct checksum, false for checksum mismatch
 */
static bool verify_checksum(const void *buffer, size_t buflen) {
    uint8_t sum = 0;

    for(int idx = 0; idx < buflen; ++idx) {
        sum += ((const uint8_t *)buffer)[idx];
    }

    return sum == 0;
}

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

    if(!verify_checksum(rsdp, ACPI_V1_RSDP_SIZE)) {
        return false;
    }

    if(rsdp->revision == ACPI_V1_REVISION) {
        return true;
    }

    if(rsdp->revision != ACPI_V2_REVISION) {
        return false;
    }

    return verify_checksum(rsdp, sizeof(acpi_rsdp_t));
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
 * 
 */
void find_acpi_rsdp(void) {
    /* At the stage of the boot process where this function is called, the memory
     * where the RSDP is located is mapped 1:1 so a pointer to the RSDP has the
     * same value as its physical address. */
    rsdp_paddr = (kern_paddr_t)find_rsdp();
}

/**
 * Verify the checksum of an ACPI table header
 *
 * @param header mapped ACPI table header
 * @param signature expected signature
 * @return true if the signature matches, false otherwise
 *
 * */
static bool verify_signature(const acpi_table_header_t *header, const char *signature) {
    return strncmp(header->signature, signature, sizeof(header->signature)) == 0;
}

/**
 * Map ACPI RSDP
 * 
 * We don't validate the contents (checksum, revision) because this has already
 * been done by find_acpi_rsdp().
 *
 * @param paddr physical memory address of ACPI RSDP
 * @return pointer to mapped RSDP on success, NULL on error
 *
 * */
static const acpi_rsdp_t *map_rsdp(uint64_t paddr) {
    return map_in_kernel(paddr, sizeof(acpi_rsdp_t), JINUE_PROT_READ);
}

/**
 * Extend the existing mapping of a table header to the full table
 *
 * @param header mapped ACPI table header
 * @return pointer to mapped table on success, NULL on error
 *
 * */
static const void *map_table(const acpi_table_header_t *header) {
    if(header->length < sizeof(acpi_table_header_t)) {
        return NULL;
    }

    if(header->length > ACPI_TABLE_MAX_SIZE) {
        return NULL;
    }

    expand_map_in_kernel(header, header->length, JINUE_PROT_READ);

    if(! verify_checksum(header, header->length)) {
        return NULL;
    }
    
    return header;
}

/**
 * Map ACPI table header
 *
 * @param paddr physical memory address of ACPI table header
 * @return pointer to mapped header on success, NULL on error
 *
 * */
static const acpi_table_header_t *map_header(kern_paddr_t paddr) {
    return map_in_kernel(paddr, sizeof(acpi_table_header_t), JINUE_PROT_READ);
}

/* Size of the fixed part of the RSDT, excluding the entries. */
#define RSDT_BASE_SIZE ((size_t)(const char *)&(((const acpi_rsdt_t *)0)->entries))

/**
 * Map the RSDT/XSDT
 *
 * @param rsdt_paddr physical memory address of RSDT/XSDT
 * @param is_xsdt whether the table is a XSDT (true) or a RSDT (false)
 * @return mapped RSDT/XSDT on success, NULL on error
 *
 * */
static const acpi_rsdt_t *map_rsdt(uint64_t rsdt_paddr, bool is_xsdt) {
    const acpi_table_header_t *header = map_header(rsdt_paddr);

    if(header == NULL) {
        return NULL;
    }

    const char *const signature = is_xsdt ? "XSDT" : "RSDT";

    if(! verify_signature(header, signature)) {
        undo_map_in_kernel((void *)header);
        return NULL;
    }

    if(header->length < RSDT_BASE_SIZE) {
        undo_map_in_kernel((void *)header);
        return NULL;
    }

    const acpi_rsdt_t *rsdt = map_table(header);

    if(rsdt == NULL) {
        undo_map_in_kernel((void *)header);
        return NULL;
    }

    return rsdt;
}

/**
 * Process the entries of the mapped RSDT/XSDT to find relevant tables
 *
 * @param rsdt mapped RSDT/XSDT
 * @param is_xsdt whether the table is a XSDT (true) or RSDT (false)
 *
 * */
void process_rsdt(const acpi_rsdt_t *rsdt, bool is_xsdt) {
    size_t entries = (rsdt->header.length - RSDT_BASE_SIZE) / sizeof(uint32_t);

    if(is_xsdt && entries % 2 != 0) {
        --entries;
    }

    for(int idx = 0; idx < entries; ++idx) {
        /* x86 is little endian */
        uint64_t paddr = rsdt->entries[idx];

        if(is_xsdt) {
            paddr |= ((uint64_t)rsdt->entries[++idx]) << 32;
        }

        const acpi_table_header_t *header = map_header(paddr);

        if(header == NULL) {
            continue;
        }

        const char *signature = "FACP";

        if(verify_signature(header, signature) && tables.fadt == NULL) {
            tables.fadt = map_table(header);
            continue;;
        }

        signature = "APIC";

        if(verify_signature(header, signature) && tables.madt == NULL) {
            tables.madt = map_table(header);
            continue;
        }

        signature = "HPET";

        if(verify_signature(header, signature) && tables.hpet == NULL) {
            tables.hpet = map_table(header);
            continue;
        }

        undo_map_in_kernel((void *)header);
    }
}

/**
 * Map the ACPI tables in the kernel's mapping area
 * 
 * @param rsdp_paddr physical memory address of the RSDP
 * 
 */
static void load_acpi_tables(kern_paddr_t rsdp_paddr) {
    memset(&tables, 0, sizeof(tables));

    if(rsdp_paddr == 0) {
        return;
    }

    const acpi_rsdp_t *rsdp = map_rsdp(rsdp_paddr);

    tables.rsdp = rsdp;

    uint64_t rsdt_paddr;
    bool is_xsdt;

    if(rsdp->revision == ACPI_V1_REVISION) {
        rsdt_paddr  = rsdp->rsdt_address;
        is_xsdt     = false;
    } else {
        /* TODO handle the case where the address > 4GB and PAE is disabled. */
        rsdt_paddr  = rsdp->xsdt_address;
        is_xsdt     = true;
    }

    const acpi_rsdt_t *rsdt = map_rsdt(rsdt_paddr, is_xsdt);

    if(rsdt == NULL) {
        return;
    }

    tables.rsdt = rsdt;

    process_rsdt(rsdt, is_xsdt);
}

// TODO delete this function
static void dump_table(const acpi_table_header_t *table, const char *name) {
    info("  %s:", name);
    info("    address:  %#p", table);

    if(table != NULL) {
        info("    revision: %" PRIu8, table->revision);
        info("    length:   %" PRIu32, table->length);
    }
}

// TODO delete this function
void dump_acpi_tables(void) {
    const acpi_table_header_t *rsdt = &tables.rsdt->header; 
    const char *rsdt_name           = (rsdt != NULL && rsdt->signature[0] == 'X') ? "XSDT" : "RSDT";

    info("ACPI tables:");
    dump_table(rsdt, rsdt_name);
    dump_table(&tables.fadt->header, "FADT");
    dump_table(&tables.madt->header, "MADT");
    dump_table(&tables.hpet->header, "HPET");
}

/**
 * Initialize ACPI
 */
void init_acpi(void) {
    load_acpi_tables(rsdp_paddr);
    dump_acpi_tables();
}

/**
 * Get the physical address of the ACPI RSDP
 *
 * @return physical address of RSDP if found, zero otherwise
 */
kern_paddr_t acpi_get_rsdp_paddr(void) {
    return rsdp_paddr;
}

/**
 * Process the ACPI tables mapped by user space
 *
 * @param tables structure with pointers to ACPI tables
 */
void machine_set_acpi_tables(const jinue_acpi_tables_t *tables) {
    /* TODO implement this */
}
