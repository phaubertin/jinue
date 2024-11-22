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

#include <kernel/infrastructure/i686/drivers/acpi.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static uint8_t compute_checksum(const void *buffer, size_t buflen) {
    uint8_t sum = 0;

    for(int idx = 0; idx < buflen; ++idx) {
        sum += ((const uint8_t *)buffer)[idx];
    }

    return sum;
}

static bool check_rsdp(const acpi_rsdp_t *rsdp) {
    const char *const signature = "RSD PTR ";

    if(strncmp(rsdp->signature, signature, strlen(signature)) != 0) {
        return false;
    }

    uint8_t checksum = compute_checksum(rsdp, sizeof(acpiv1_rsdp_t));

    if(checksum != 0) {
        return false;
    }

    if(rsdp->revision == ACPI_V1_REVISION) {
        return true;
    }

    if(rsdp->revision != ACPI_V2_REVISION) {
        return false;
    }

    return compute_checksum(rsdp, sizeof(acpi_rsdp_t)) == 0;
}

const acpi_rsdp_t *acpi_find_rsdp(void) {
    
    const char *ebda = (const char *)(16 * (*(uint16_t *)ACPI_BDA_EBDA));

    for(const char *addr = ebda; addr < ebda + 1024; addr += 16) {
        const acpi_rsdp_t *rsdp = (const acpi_rsdp_t *)addr;

        if(check_rsdp(rsdp)) {
            return rsdp;
        }
    }

    const char *const start = (const char *)0x0e0000;
    const char *const end   = (const char *)0x100000;

    for(const char *addr = start; addr < end; addr += 16) {
        const acpi_rsdp_t *rsdp = (const acpi_rsdp_t *)addr;

        if(check_rsdp(rsdp)) {
            return rsdp;
        }
    }

    return NULL;
}
