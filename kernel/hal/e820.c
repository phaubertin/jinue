/*
 * Copyright (C) 2019 Philippe Aubertin.
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

#include <hal/boot.h>
#include <hal/e820.h>
#include <printk.h>

bool e820_is_valid(const e820_t *e820_entry) {
    return e820_entry->size != 0;
}

bool e820_is_available(const e820_t *e820_entry) {
    return e820_entry->type == E820_RAM;
}

const char *e820_type_description(e820_type_t type) {
    switch(type) {
    
    case E820_RAM:
        return "available";
    
    case E820_RESERVED:
        return "unavailable/reserved";
    
    case E820_ACPI:
        return "unavailable/acpi";
    
    default:
        return "unavailable/other";
    }    
}

void e820_dump(void) {
    unsigned int idx;
    
    printk("Dump of the BIOS memory map:\n");

    const boot_info_t *boot_info = get_boot_info();

    for(idx = 0; idx < boot_info->e820_entries; ++idx) {
        const e820_t *e820_entry = &boot_info->e820_map[idx];

        printk("%c [%q-%q] %s\n",
            e820_is_available(e820_entry)?'*':' ',
            e820_entry->addr,
            e820_entry->addr + e820_entry->size - 1,
            e820_type_description(e820_entry->type)
        );
    }
}
