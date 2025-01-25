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

#include <kernel/infrastructure/i686/firmware/bios.h>
#include <kernel/utils/asm/utils.h>
#include <stdlib.h>

/**
 * Get address of the Extended BIOS Data Area (EBDA)
 * 
 * The returned address is guaranteed to be aligned on a 16-byte boundary. This
 * information is read from the Bios Data Area (BDA). This function must be
 * called early in the boot process while conventional memory is still mapped
 * 1:1 in virtual memory.
 * 
 * @return address of EBDA, 0 for none or if it could not be determined
 */
uint32_t get_bios_ebda_addr(void) {
    uintptr_t ebda = 16 * (*(uint16_t *)BIOS_BDA_EBDA_SEGMENT);

    if(ebda < 0x80000 || ebda >= 0xa0000) {
        return NULL;
    }

    return ebda;
}

/**
 * Get the base (aka. conventional) memory size from the BIOS
 * 
 * The returned size is guaranteed to be a multiple of 1 kB. This information
 * is read from the Bios Data Area (BDA). This function must be called early in
 * the boot process while conventional memory is still mapped 1:1 in virtual
 * memory.
 * 
 * @return base memory size, 0 if it could not be determined
 */
size_t get_bios_base_memory_size(void) {
    size_t size_kb = *(uint16_t *)BIOS_BDA_MEMORY_SIZE;

    if(size_kb < 512 || size_kb > 640) {
        return 0;
    }

    return size_kb * KB;
}
