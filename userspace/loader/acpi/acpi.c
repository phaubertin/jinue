/*
 * Copyright (C) 2024 Philippe Aubertin.
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

#include <jinue/jinue.h>
#include <jinue/utils.h>
#include <sys/auxv.h>
#include <sys/mman.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "../debug.h"
#include "acpi.h"

/**
 * Verify the checksum of an ACPI data structure
 *
 * @param buffer pointer to ACPI data structure
 * @param buflen size of ACPI data structure
 * @return true for correct checksum, false for checksum mismatch
 *
 * */
static bool verify_checksum(const void *buffer, size_t buflen) {
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
static bool verify_signature(const acpi_header_t *header, const char *signature) {
    return strncmp(header->signature, signature, sizeof(header->signature)) == 0;
}

/**
 * Map an ACPI data structure
 *
 * @param paddr physical memory address of data structure
 * @param size size of data structure
 * @return pointer to mapped data on success, NULL on error
 *
 * */
static const void *map_size(uint64_t paddr, size_t size) {
    size_t offset = paddr % JINUE_PAGE_SIZE;

    size    += offset;
    paddr   -= offset;

    void *mapped = mmap(NULL, size, PROT_READ, MAP_SHARED, 0, paddr);

    if(mapped == MAP_FAILED) {
        return NULL;
    }

    return (const char *)mapped + offset;
}

/**
 * Map ACPI RSDP
 * 
 * We don't validate the contents (checksum, revision) because we assume the
 * kernel has done so before setting the address in the auxiliary vector.
 *
 * @param paddr physical memory address of ACPI RSDP
 * @return pointer to mapped RSDP on success, NULL on error
 *
 * */
static const acpi_rsdp_t *map_rsdp(uint64_t paddr) {
    const acpi_rsdp_t *rsdp = map_size(paddr, ACPI_V1_RSDP_SIZE);

    if(rsdp == NULL || rsdp->revision == ACPI_V1_REVISION) {
        return rsdp;
    }

    size_t offset = paddr % JINUE_PAGE_SIZE;

    if(JINUE_PAGE_SIZE - offset >= sizeof(acpi_rsdp_t)) {
        return rsdp;
    }

    /* Here, we rely on the fact that our implementation of mmap() allocates
     * virtual memory sequentially to simply extend the existing mapping. */
    size_t extsize          = sizeof(acpi_rsdp_t) - (JINUE_PAGE_SIZE - offset);
    const void *extension   = map_size(paddr - offset + JINUE_PAGE_SIZE, extsize);

    if(extension == NULL) {
        return NULL;
    }
    
    return rsdp;
}

/**
 * Map ACPI table header
 *
 * @param paddr physical memory address of ACPI table header
 * @return pointer to mapped header on success, NULL on error
 *
 * */
static const acpi_header_t *map_header(uint64_t paddr) {
    return map_size(paddr, sizeof(acpi_header_t));
}

/**
 * Extend the existing mapping of a table header to the full table
 * 
 * This function relies on the fact that our implementation of mmap() allocates
 * virtual memory sequentially to extend an existing mapping. It assumes that
 * mmap() wasn't called since the call to map_header() that mapped the table
 * header passed as agument.
 *
 * @param paddr physical memory address of the ACPI table
 * @param header mapped ACPI table header
 * @param name table name, for log messages
 * @return pointer to mapped table on success, NULL on error
 *
 * */
static const void *map_table(uint64_t paddr, const acpi_header_t *header, const char *name) {
    if(header->length < sizeof(acpi_header_t)) {
        jinue_warning("Value of ACPI table length member is too small (%" PRIu32 ", %s)", name);
        return NULL;
    }

    if(header->length > ACPI_TABLE_MAX_SIZE) {
        jinue_warning("Value of ACPI table length member is too large (%" PRIu32 ", %s)", name);
        return NULL;
    }

    size_t offset       = paddr % JINUE_PAGE_SIZE;
    size_t allocated    = JINUE_PAGE_SIZE - offset;

    if(allocated < sizeof(acpi_header_t)) {
        allocated += JINUE_PAGE_SIZE;
    }

    if(header->length > allocated) {
        /* Here, we rely on the fact that our implementation of mmap() allocates
         * virtual memory sequentially to simply extend the existing mapping. */
        size_t extsize          = header->length - allocated;
        const void *extension   = map_size(paddr + allocated, extsize);

        if(extension == NULL) {
            jinue_warning("Failed mapping ACPI table (%s)", name);
            return NULL;
        }
    }

    if(! verify_checksum(header, header->length)) {
        jinue_warning("ACPI table checksum mismatch (%s)", name);
        return NULL;
    }
    
    return header;
}

/* Size of the fixed part of the RSDT, excluding hte entries. */
#define RSDT_BASE_SIZE ((size_t)(const char *)&(((const acpi_rsdt_t *)0)->entries))

/**
 * Map the RSDT/XSDT
 *
 * @param paddr physical memory address of RSDT/XSDT
 * @param is_xsdt whether the table is a XSDT (true) or a RSDT (false)
 * @return mapped RSDT/XSDT on success, NULL on error
 *
 * */
static const acpi_rsdt_t *map_rsdt(uint64_t paddr, bool is_xsdt) {
    const acpi_header_t *header = map_header(paddr);

    if(header == NULL) {
        return NULL;
    }

    const char *const signature = is_xsdt ? "XSDT" : "RSDT";

    if(! verify_signature(header, signature)) {
        jinue_warning("Signature mismatch for ACPI %s", signature);
        return NULL;
    }

    if(header->length < RSDT_BASE_SIZE) {
        jinue_warning("ACPI %s table is too small", signature);
        return NULL;
    }

    return map_table(paddr, header, signature);
}

/**
 * Process the entries of the mapped RSDT/XSDT to find relevant tables
 *
 * @param tables tables structure (output)
 * @param rsdt mapped RSDT/XSDT
 * @param is_xsdt whether the table is a XSDT (true) or RSDT (false)
 *
 * */
void process_rsdt(jinue_acpi_tables_t *tables, const acpi_rsdt_t *rsdt, bool is_xsdt) {
    size_t entries = (rsdt->length - RSDT_BASE_SIZE) / sizeof(uint32_t);

    if(is_xsdt && entries % 2 != 0) {
        --entries;
    }

    for(int idx = 0; idx < entries; ++idx) {
        /* x86 is little endian */
        uint64_t paddr = rsdt->entries[idx];

        if(is_xsdt) {
            paddr |= ((uint64_t)rsdt->entries[++idx]) << 32;
        }

        const acpi_header_t *header = map_header(paddr);

        if(header == NULL) {
            continue;
        }

        const char *signature = "FACP";

        if(verify_signature(header, signature) && tables->fadt == NULL) {
            tables->fadt = map_table(paddr, header, "FADT");
        }

        signature = "APIC";

        if(verify_signature(header, signature) && tables->madt == NULL) {
            tables->madt = map_table(paddr, header, "MADT");
        }
    }
}

/**
 * Map the RSDT/XSDT and then iterate over its entries to find relevant tables
 *
 * @param tables tables structure (output)
 * @param paddr physical memory address of RSDT/XSDT
 * @param is_xsdt whether the table is a XSDT (true) or RSDT (false)
 *
 * */
static void load_rsdt(jinue_acpi_tables_t *tables, uint64_t paddr, bool is_xsdt) {
    const acpi_rsdt_t *rsdt = map_rsdt(paddr, is_xsdt);

    if(rsdt == NULL) {
        return;
    }

    tables->rsdt = rsdt;
    process_rsdt(tables, rsdt, is_xsdt);
}

/**
 * Map the RSDP/XSDT and then call load_rsdt() with the RSDT/XSDT address
 *
 * @param tables tables structure (output)
 * @param rsdp_paddr physical memory address of the RSDP
 *
 * */
static void load_rsdp(jinue_acpi_tables_t *tables, uint32_t rsdp_paddr) {
    const acpi_rsdp_t *rsdp = map_rsdp(rsdp_paddr);

    if(rsdp == NULL) {
        return;
    }

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

    load_rsdt(tables, rsdt_paddr, is_xsdt);
}

/**
 * Map relevant ACPI tables and report to kernel
 * 
 * Map the ACPI tables needed by the kernel in memory, set the pointers to them
 * in a tables structure (jinue_acpi_tables_t) and call the kernel with this
 * information.
 *
 * @return EXIT_SUCCESS on success, EXIT_FAILURE on error
 *
 * */
int load_acpi_tables(void) {
    jinue_acpi_tables_t tables;
    tables.rsdt = NULL;
    tables.fadt = NULL;
    tables.madt = NULL;

    uint32_t rsdp_paddr = getauxval(JINUE_AT_ACPI_RSDP);

    /* If the kernel set this auxiliary vector to zero, it knows the RSDP is
     * nowhere to be found and doesn't expect to be called. Since this is
     * expected, it is not a failure (i.e. we return EXIT_SUCCESS).
     * 
     * In any other situation, the kernel does expect to be called with our
     * best effort to map the tables so it can complete its initialization and
     * it will deal with NULL entries in the tables structure if need be. */
    if(rsdp_paddr == 0) {
        return EXIT_SUCCESS;
    }

    load_rsdp(&tables, rsdp_paddr);

    dump_acpi_tables(&tables);

    int status = jinue_acpi(&tables, &errno);

    if(status != 0) {
        jinue_error("error: ACPI call failed: %s", strerror(errno));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
