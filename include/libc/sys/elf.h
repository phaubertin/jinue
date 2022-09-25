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

#ifndef _JINUE_LIBC_SYS_ELF_H
#define _JINUE_LIBC_SYS_ELF_H

#include <jinue/shared/asm/auxv.h>
#include <sys/asm/elf.h>
#include <stdint.h>

typedef uint32_t Elf32_Addr;

typedef uint16_t Elf32_Half;

typedef uint32_t Elf32_Off;

typedef int32_t Elf32_Sword;

typedef uint32_t Elf32_Word;

typedef struct {
        unsigned char	e_ident[EI_NIDENT];
        Elf32_Half    	e_type;
        Elf32_Half    	e_machine;
        Elf32_Word    	e_version;
        Elf32_Addr    	e_entry;
        Elf32_Off     	e_phoff;
        Elf32_Off     	e_shoff;
        Elf32_Word    	e_flags;
        Elf32_Half    	e_ehsize;
        Elf32_Half    	e_phentsize;
        Elf32_Half    	e_phnum;
        Elf32_Half    	e_shentsize;
        Elf32_Half    	e_shnum;
        Elf32_Half		e_shstrndx;
} Elf32_Ehdr;

typedef struct {
        Elf32_Word 		p_type;
        Elf32_Off  		p_offset;
        Elf32_Addr 		p_vaddr;
        Elf32_Addr 		p_paddr;
        Elf32_Word 		p_filesz;
        Elf32_Word 		p_memsz;
        Elf32_Word 		p_flags;
        Elf32_Word 		p_align;
} Elf32_Phdr;

typedef struct {
        Elf32_Word 		sh_name;
        Elf32_Word 		sh_type;
        Elf32_Word 		sh_flags;
        Elf32_Addr 		sh_addr;
        Elf32_Off  		sh_offset;
        Elf32_Word 		sh_size;
        Elf32_Word 		sh_link;
        Elf32_Word 		sh_info;
        Elf32_Word 		sh_addralign;
        Elf32_Word 		sh_entsize;
} Elf32_Shdr;

typedef struct {
        Elf32_Word 		st_name;
        Elf32_Addr 		st_value;
        Elf32_Word 		st_size;
        unsigned char   st_info;
        unsigned char   st_other;
        Elf32_Half      st_shndx;
} Elf32_Sym;

typedef struct {
    int a_type;
    union {
        uint32_t a_val;
    } a_un;
} Elf32_auxv_t;

typedef Elf32_auxv_t auxv_t;

#endif
