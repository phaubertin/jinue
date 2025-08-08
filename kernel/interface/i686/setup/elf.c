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

#include <kernel/interface/i686/setup/alloc.h>
#include <kernel/interface/i686/setup/elf.h>
#include <kernel/interface/i686/setup/linkdefs.h>
#include <kernel/interface/i686/types.h>
#include <kernel/machine/asm/machine.h>
#include <kernel/utils/utils.h>
#include <sys/elf.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * Perform sanity check on ELF header
 * 
 * @param ehdr ELF header
 * @return true if the header is likely valid, false if definitely invalid
 */
static bool check_header(const Elf32_Ehdr *ehdr) {
    if(ehdr == NULL) {
        return false;
    }

    bool valid = true;
    
    valid &= ehdr->e_ident[EI_MAG0] == ELF_MAGIC0;
    valid &= ehdr->e_ident[EI_MAG1] == ELF_MAGIC1;
    valid &= ehdr->e_ident[EI_MAG2] == ELF_MAGIC2;
    valid &= ehdr->e_ident[EI_MAG3] == ELF_MAGIC3;
    valid &= ehdr->e_ident[EI_CLASS] == ELFCLASS32;
    valid &= ehdr->e_ident[EI_DATA] == ELFDATA2LSB;
    valid &= ehdr->e_phnum != 0;
    valid &= ehdr->e_phoff != 0;
    valid &= ehdr->e_phentsize == sizeof(Elf32_Phdr);

    return valid;
}

/**
 * Get program header table
 *
 * @param ehdr ELF file header
 * @return program header table
 */
static const Elf32_Phdr *program_header_table(const Elf32_Ehdr *ehdr) {
    return (Elf32_Phdr *)((char *)ehdr + ehdr->e_phoff);
}

/**
 * Get program header for writable segment
 *
 * @param elf ELF header
 * @return program header if found, NULL otherwise
 */
static const Elf32_Phdr *writable_program_header(const Elf32_Ehdr *ehdr) {
    const Elf32_Phdr *phdr = program_header_table(ehdr);

    for(int idx = 0; idx < ehdr->e_phnum; ++idx) {
        if(phdr[idx].p_type != PT_LOAD) {
            continue;
        }

        if(phdr[idx].p_flags & PF_W) {
            return &phdr[idx];
        }
    }

    return NULL;
}

/**
 * Load the kernel data segment from its ELF binary
 * 
 * @param alloc_ptr allocation pointer, i.e. where to allocate memory
 * @param bootinfo boot information structure
 * @return updated allocation pointer
 */
void prepare_data_segment(data_segment_t *segment, bootinfo_t *bootinfo) {
    const Elf32_Ehdr *ehdr = bootinfo->kernel_start;

    segment->physaddr = 0;
    segment->size = 0;
    segment->start = NULL;

    if(!check_header(ehdr)) {
        return;
    }

    const Elf32_Phdr *phdr = writable_program_header(ehdr);

    if(phdr == NULL) {
        return;
    }

    if(phdr->p_vaddr == 0 || phdr->p_memsz == 0) {
        return;
    }

    segment->start = (void *)phdr->p_vaddr;
    segment->size  = ALIGN_END(phdr->p_memsz, PAGE_SIZE);

    const char *src = (char *)ehdr + phdr->p_offset;
    char *dest      = alloc_pages(bootinfo, segment->size);

    for(int idx = 0; idx < segment->size; ++idx) {
        dest[idx] = (idx < phdr->p_filesz) ? src[idx] : 0;
    }

    segment->physaddr = (size_t)dest;
}

/**
 * Get program header for executable segment
 *
 * @param elf ELF header
 * @return program header if found, NULL otherwise
 */
const Elf32_Phdr *executable_program_header(const Elf32_Ehdr *ehdr) {
    const Elf32_Phdr *phdr = program_header_table(ehdr);

    for(int idx = 0; idx < ehdr->e_phnum; ++idx) {
        if(phdr[idx].p_type != PT_LOAD) {
            continue;
        }

        if(phdr[idx].p_flags & PF_X) {
            return &phdr[idx];
        }
    }

    return NULL;
}

/**
 * Get ELF program header for kernel code
 *
 * @param bootinfo boot information structure
 * @return program header if found, NULL otherwise
 */
const Elf32_Phdr *kernel_code_program_header(const bootinfo_t *bootinfo) {
    const Elf32_Ehdr *ehdr = bootinfo->kernel_start;

    if(!check_header(ehdr)) {
        return NULL;
    }

    return executable_program_header(ehdr);
}

/**
 * Get entry point of kernel ELF binary
 *
 * @param bootinfo boot information structure
 * @return entry point virtual address
 */
Elf32_Addr get_kernel_entry_point(const bootinfo_t *bootinfo) {
    const Elf32_Ehdr *ehdr = bootinfo->kernel_start;
    return ehdr->e_entry;
}
