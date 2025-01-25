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
#include <kernel/domain/services/logging.h>
#include <kernel/domain/services/mman.h>
#include <kernel/infrastructure/acpi/acpi.h>
#include <kernel/infrastructure/i686/firmware/bios.h>
#include <kernel/infrastructure/i686/firmware/mp.h>
#include <kernel/infrastructure/i686/types.h>
#include <kernel/utils/utils.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

#define MAXIMUM_TABLE_SIZE (16 * KB)

#define PADDR_NULL 0

static struct  {
    const mp_ptr_struct_t   *ptrst;
    const mp_conf_table_t   *table;
    uint32_t                 ptrst_paddr;
} mp;

/**
 * Validate the floating pointer structure
 * 
 * @param ptrst pointer to floating pointer structure candidate
 * @return true if valid, false otherwise
 */
static bool validate_pointer_structure(const mp_ptr_struct_t *ptrst) {
    if(strncmp(ptrst->signature, MP_FLOATING_PTR_SIGNATURE, sizeof(ptrst->signature)) != 0) {
        return false;
    }

    if(16 * ptrst->length != sizeof(*ptrst)) {
        return false;
    }

    /* The specification uses the same checksum algorithm as ACPI. */
    return verify_acpi_checksum(ptrst, sizeof(*ptrst));
}

/**
 * Validate the header of the configuration table
 * 
 * When this function is called, the configuration table is partially mapped,
 * so only the header fields can be validated. Specifically, the checksum
 * cannot be checked, so this needs to be done separately once the table is
 * fully mapped. One validation check performed by this function is that the
 * value in the base table size field is reasonable and we don't want to rely
 * on this value to map the full table before this is confirmed.
 * 
 * @param table pointer to configuration table candidate
 * @return true if valid, false otherwise
 */
static bool validate_configuration_table_header(const mp_conf_table_t *table) {
    if(strncmp(table->signature, MP_CONF_TABLE_SIGNATURE, sizeof(table->signature)) != 0) {
        return false;
    }

    if(table->base_length < sizeof(*table) || table->base_length > MAXIMUM_TABLE_SIZE) {
        return false;
    }

    return table->revision == MP_REVISION_V1_1 || table->revision == MP_REVISION_V1_4;
}

/**
 * Scan a range of physical memory to find the floating pointer structure
 * 
 * The start and end addresses must both be aligned on a 16-byte boundary.
 *
 * @param from start address of scan
 * @param to end address of scan
 * @return physical address of structure, PADDR_NULL if not found
 */
static uint32_t scan_address_range(uint32_t from, uint32_t to) {
    for(uint32_t addr = from; addr < to; addr += 16) {
        /* At the stage of the boot process where this function is called, the
         * memory where the floating pointer structure can be located is mapped
         * 1:1 so a pointer to the structure has the same value as its physical
         * address.*/
        const mp_ptr_struct_t *ptrst = (const mp_ptr_struct_t *)addr;

        if(validate_pointer_structure(ptrst)) {
            return addr;
        }
    }

    return PADDR_NULL;
}

/**
 * Scan memory for the floating pointer structure
 * 
 * The ranges where the structures can be located are defined in section 4 of
 * the Multiprocessor Specification:
 * 
 * " a. In the first kilobyte of Extended BIOS Data Area (EBDA), or
 *   b. Within the last kilobyte of system base memory (e.g., 639K-640K for
 *      systems with 640 KB of base memory or 511K-512K for systems with 512 KB
 *      of base memory) if the EBDA segment is undefined, or
 *   c. In the BIOS ROM address space between 0F0000h and 0FFFFFh. "
 *
 * @return physical address of structure, PADDR_NULL if not found
 */
static uint32_t scan_for_pointer_structure(void) {
    uint32_t ebda = get_bios_ebda_addr();

    if(ebda != 0) {
        uint32_t ptrst = scan_address_range(ebda, ebda + KB);

        if(ptrst != PADDR_NULL) {
            return ptrst;
        }
    } else {
        uint32_t ptrst  = PADDR_NULL;
        uint32_t memtop = get_bios_base_memory_size();

        if(memtop != 0) {
            ptrst = scan_address_range(memtop - KB, memtop);
        }

        if(ptrst != PADDR_NULL) {
            return ptrst;
        }
    }

    return scan_address_range(0xf0000, 0x100000);
}

/**
 * Find the floating pointer structure in memory
 * 
 * At the stage of the boot process where this function is called, the memory
 * where the floating pointer structure can be located has to be mapped 1:1 so
 * a pointer to the floating pointer structure has the same value as its
 * physical address.
 * 
 * @param paddr physical address of the structure, PADDR_NULL if not found
 */
void find_mp(void) {
    mp.ptrst_paddr = scan_for_pointer_structure();
}

/**
 * Map the floating pointer structure
 * 
 * @param paddr physical address of the structure, PADDR_NULL if not found
 */
static const mp_ptr_struct_t *map_pointer_structure(uint32_t paddr) {
    if(paddr == PADDR_NULL) {
        return NULL;
    }

    return map_in_kernel(
        paddr,
        sizeof(mp_ptr_struct_t),
        JINUE_PROT_READ
    );
}

/**
 * Map the configuration table
 * 
 * @param ptrst pointer to mapped floating pointer structure, NULL if not found
 */
static const mp_conf_table_t *map_configuration_table(const mp_ptr_struct_t *ptrst) {
    if(ptrst == NULL || ptrst->addr == 0) {
        return NULL;
    }

    const mp_conf_table_t *table = map_in_kernel(
        ptrst->addr,
        sizeof(mp_conf_table_t),
        JINUE_PROT_READ
    );

    if(! validate_configuration_table_header(table)) {
        undo_map_in_kernel();
        return NULL;
    }

    resize_map_in_kernel(table->base_length);

    if(! verify_acpi_checksum(table, table->base_length)) {
        undo_map_in_kernel();
        return NULL;
    }

    return table;  
}

/**
 * Log information regarding Multiprocessor Specification (MP) data structures
 */
static void report_mp_info(void) {
    info("Multiprocessor Specification (MP):");

    const char *float_ptr = "Floating pointer structure";

    if(mp.ptrst_paddr == PADDR_NULL) {
        info("  %s not found", float_ptr);
    } else {
        info("  %s found at address %#" PRIx32, float_ptr, mp.ptrst_paddr);
    }

    if(mp.table != NULL) {
        info(
            "  Configuration table version 1.%d at address %#" PRIx32,
            mp.table->revision == MP_REVISION_V1_1 ? 1 : 4,
            mp.ptrst->addr
        );
    }
}

/**
 * Map Intel Multiprocessor Specification (MP) data structures
 */
void init_mp(void) {
    mp.ptrst = map_pointer_structure(mp.ptrst_paddr);
    mp.table = map_configuration_table(mp.ptrst);
    report_mp_info();
}
