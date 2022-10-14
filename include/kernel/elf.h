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

#ifndef JINUE_KERNEL_ELF_H
#define JINUE_KERNEL_ELF_H

#include <sys/elf.h>
#include <types.h>


typedef struct {
    void            *entry;
    void            *stack_addr;
    addr_t           at_phdr;
    int              at_phent;
    int              at_phnum;
    addr_space_t    *addr_space;
    const char      *argv0;
} elf_info_t;

typedef struct {
    Elf32_Addr  addr;
    const char *name;
} elf_symbol_t;


static inline const char *elf_file_bytes(const Elf32_Ehdr *elf_header) {
    return (const char *)elf_header;
}

static inline const Elf32_Shdr *elf_get_section_header(const Elf32_Ehdr *elf_header, int index) {
    const char *elf_file        = elf_file_bytes(elf_header);
    const char *section_table   = &elf_file[elf_header->e_shoff];
    
    return (const Elf32_Shdr *)&section_table[index * elf_header->e_shentsize];
}


bool elf_check(Elf32_Ehdr *elf);

const Elf32_Phdr *elf_executable_program_header(const Elf32_Ehdr *elf);

void elf_load(
        elf_info_t      *info,
        Elf32_Ehdr      *elf,
        const char      *argv0,
        const char      *cmdline,
        addr_space_t    *addr_space,
        boot_alloc_t    *boot_alloc);

void elf_allocate_stack(elf_info_t *info, boot_alloc_t *boot_alloc);

void elf_initialize_stack(elf_info_t *info, const char *cmdline);

const Elf32_Shdr *elf_find_section_header_by_type(
        const Elf32_Ehdr    *elf_header,
        Elf32_Word           type);

int elf_find_symbol_by_address_and_type(
        const Elf32_Ehdr    *elf_header,
        Elf32_Addr           addr,
        int                  type,
        elf_symbol_t        *result);

#endif
