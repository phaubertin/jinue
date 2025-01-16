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
#include <kernel/domain/services/mman.h>
#include <kernel/infrastructure/acpi/asm/acpi.h>
#include <kernel/infrastructure/acpi/acpi.h>
#include <kernel/infrastructure/acpi/tables.h>
#include <stdint.h>
#include <string.h>

/**
 * Verify the checksum of an ACPI data structure
 *
 * @param buffer pointer to ACPI data structure
 * @param buflen size of ACPI data structure
 * @return true for correct checksum, false for checksum mismatch
 */
bool verify_acpi_checksum(const void *buffer, size_t buflen) {
    uint8_t sum = 0;

    for(int idx = 0; idx < buflen; ++idx) {
        sum += ((const uint8_t *)buffer)[idx];
    }

    return sum == 0;
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

    if(! verify_acpi_checksum(header, header->length)) {
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

/** Size of the fixed part of the RSDT, excluding the entries. */
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
 * Match a table to a table definition
 * 
 * If there is a match, map the table and assign it. Otherwise, unmap the
 * header.
 * 
 * @param header table header
 * @param table_defs table definitions array terminated by a NULL signature
 */
static void match_table(const acpi_table_header_t *header, const acpi_table_def_t *table_defs) {
    for(int idx = 0; table_defs[idx].signature != NULL; ++idx) {
        const acpi_table_def_t *def = &table_defs[idx];
        
        if(*def->ptr == NULL && verify_signature(header, def->signature)) {
            *def->ptr = map_table(header);
            return;
        }
    }

    undo_map_in_kernel((void *)header);
}

/**
 * Process the entries of the mapped RSDT/XSDT to find relevant tables
 *
 * @param rsdt mapped RSDT/XSDT
 * @param is_xsdt whether the table is a XSDT (true) or RSDT (false)
 * @param table_defs table definitions array terminated by a NULL signature
 *
 * */
static void process_rsdt(const acpi_rsdt_t *rsdt, bool is_xsdt, const acpi_table_def_t *table_defs) {
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

        match_table(header, table_defs);
    }
}

/**
 * Map the ACPI tables in the kernel's mapping area
 * 
 * @param rsdp_paddr physical memory address of the RSDP
 * @param table_defs table definitions array terminated by a NULL signature
 */
void map_acpi_tables(kern_paddr_t rsdp_paddr, const acpi_table_def_t *table_defs) {
    const acpi_rsdp_t *rsdp = map_rsdp(rsdp_paddr);

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

    process_rsdt(rsdt, is_xsdt, table_defs);
}
