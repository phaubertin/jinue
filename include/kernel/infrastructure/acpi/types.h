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

#ifndef JINUE_KERNEL_INFRASTRUCTURE_ACPI_TYPES_H
#define JINUE_KERNEL_INFRASTRUCTURE_ACPI_TYPES_H

#include <stdint.h>

typedef struct {
    uint64_t addr;
    uint64_t size;
    uint32_t type;
} acpi_addr_range_t;

typedef struct {
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

typedef struct {
    acpi_table_header_t header;
    uint32_t            entries[];
} acpi_rsdt_t;

typedef struct {
    acpi_table_header_t header;
    /* TODO */
} acpi_fadt_t;

typedef struct {
    acpi_table_header_t header;
    /* TODO */
} acpi_madt_t;

typedef struct {
    acpi_table_header_t header;
    /* TODO */
} acpi_hpet_t;

#endif
