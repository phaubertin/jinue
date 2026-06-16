/*
 * Copyright (C) 2019-2026 Philippe Aubertin.
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

#include <kernel/domain/services/logging.h>
#include <kernel/domain/services/panic.h>
#include <kernel/infrastructure/i686/pmap/pmap.h>
#include <kernel/infrastructure/elf.h>
#include <kernel/interface/i686/bootinfo.h>
#include <stddef.h>

/* There is no extern declaration of this global variable in any header file but
 * it is set in kernel/i686/crt.asm. */
bootinfo_t *bootinfo;

/**
 * Get the boot information structure
 * 
 * @return boot information structure
 */
const bootinfo_t *get_bootinfo(void) {
    return bootinfo;
}

/**
 * Get description string for validation error, if any
 * 
 * @param bootinfo boot information structure
 * @return NULL if structure is valid, error string otherwise
 */
static const char *get_validation_error(const bootinfo_t *bootinfo) {
    if(bootinfo == NULL) {
        return "Boot information structure pointer is NULL.";
    }
    
    if(bootinfo->setup_signature != BOOT_SETUP_MAGIC) {
        return "Bad setup header signature.";
    }

    if(page_offset_of(bootinfo->image_start) != 0) {
        return "Kernel image start is not aligned on a page boundary";
    }

    if(page_offset_of(bootinfo->image_top) != 0) {
        return "Top of kernel image is not aligned on a page boundary";
    }

    if(page_offset_of(bootinfo->kernel_start) != 0) {
        return "Kernel ELF binary is not aligned on a page boundary";
    }

    return NULL;
}

/**
 * Check whether the boot information structure is valid
 * 
 * @return true if valid, false otherwise
 */
bool is_bootinfo_valid(void) {
    return get_validation_error(bootinfo) == NULL;
}

/**
 * Ensure the boot information structure is valid, panic otherwise
 */
void validate_bootinfo(void) {
    const char *error_description = get_validation_error(bootinfo);

    if(error_description == NULL) {
        return;
    }

    error(ERROR "validation error: %s", error_description);
    panic("Boot information structure is invalid");
}

/**
 * Get the address and size of the kernel ELF file
 * 
 * @param kernel (OUT) pointer to result structure
 */
void bootinfo_get_kernel_exec_file(exec_file_t *kernel) {
    kernel->start   = bootinfo->kernel_start;
    kernel->size    = bootinfo->kernel_size;

    if(kernel->start == NULL) {
        panic("malformed boot image: no kernel ELF binary");
    }

    if(kernel->size < sizeof(Elf32_Ehdr)) {
        panic("kernel too small to be an ELF binary");
    }

    if(!elf_check(bootinfo->kernel_start)) {
        panic("kernel ELF binary is invalid");
    }
}

/**
 * Get the address and size of the user space loader ELF file
 * 
 * @param loader (OUT) pointer to result structure
 */
void machine_get_loader(exec_file_t *loader) {
    loader->start   = bootinfo->loader_start;
    loader->size    = bootinfo->loader_size;

    if(loader->start == NULL) {
        panic("malformed boot image: no user space loader ELF binary");
    }

    if(loader->size < sizeof(Elf32_Ehdr)) {
        panic("user space loader too small to be an ELF binary");
    }

    if(!elf_check(bootinfo->kernel_start)) {
        panic("user space loader ELF binary is invalid");
    }
}

/**
 * Get the address and size of the RAM disk
 * 
 * @param loader (OUT) pointer to result structure
 */
void machine_get_ramdisk(kern_mem_block_t *ramdisk) {
    ramdisk->start  = bootinfo->ramdisk_start;
    ramdisk->size   = bootinfo->ramdisk_size;

    if(ramdisk->start == 0 || ramdisk->size == 0) {
        panic("No initial RAM disk loaded.");
    }
}
