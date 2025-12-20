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

#include <kernel/infrastructure/i686/drivers/lapic.h>
#include <kernel/infrastructure/i686/firmware/acpi.h>
#include <kernel/infrastructure/i686/firmware/mp.h>
#include <kernel/infrastructure/i686/platform.h>

/**
 * Detect presence of VGA
 * 
 * @return true if present, false otherwise
 */
bool platform_is_vga_present(void) {
    return acpi_is_vga_present();
}

/**
 * Determine the physical address of each CPU's local APIC
 * 
 * @return address of local APIC, PLATFORM_UNKNOWN_LOCAL_APIC_ADDR if unknown
 */
paddr_t platform_get_local_apic_address(void) {
    paddr_t addr = acpi_get_local_apic_address();

    if(addr != UNKNOWN_LOCAL_APIC_ADDR) {
        return addr;
    }

    addr = mp_get_local_apic_addr();

    if(addr != UNKNOWN_LOCAL_APIC_ADDR) {
        return addr;
    }

    return APIC_INIT_ADDR;
}
