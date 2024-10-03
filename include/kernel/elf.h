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

#include <kernel/types.h>
#include <sys/elf.h>


typedef struct {
    void            *entry;
    void            *stack_addr;
    addr_t           at_phdr;
    int              at_phent;
    int              at_phnum;
    addr_space_t    *addr_space;
} elf_info_t;

bool elf_check(Elf32_Ehdr *elf);

const Elf32_Phdr *elf_executable_program_header(const Elf32_Ehdr *elf);

void elf_load(
        elf_info_t      *elf_info,
        Elf32_Ehdr      *ehdr,
        const char      *argv0,
        const char      *cmdline,
        process_t       *process,
        boot_alloc_t    *boot_alloc);

const char *elf_symbol_name(const Elf32_Ehdr *ehdr, const Elf32_Sym *symbol_header);

const Elf32_Sym *elf_find_function_symbol_by_address(const Elf32_Ehdr *ehdr, Elf32_Addr addr);

#endif
