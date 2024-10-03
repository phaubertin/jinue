/*
 * Copyright (C) 2023 Philippe Aubertin.
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

#include <jinue/jinue.h>
#include <jinue/util.h>
#include <sys/elf.h>
#include <stdbool.h>
#include <stdlib.h>
#include "elf.h"

typedef struct {
    void    *entry;
    void    *stack_addr;
    void    *at_phdr;
    int      at_phent;
    int      at_phnum;
} elf_info_t;


static int check_elf_header(const Elf32_Ehdr *ehdr, size_t size) {
    if(size < sizeof(Elf32_Ehdr)) {
        jinue_error("error: init program is too small to be an ELF binary");
        return EXIT_FAILURE;
    }

    /* TODO should we check e_ehsize? */

    if(     ehdr->e_ident[EI_MAG0] != ELF_MAGIC0 ||
            ehdr->e_ident[EI_MAG1] != ELF_MAGIC1 ||
            ehdr->e_ident[EI_MAG2] != ELF_MAGIC2 ||
            ehdr->e_ident[EI_MAG3] != ELF_MAGIC3 ) {
        jinue_error("error: init program is not an ELF binary");
        return EXIT_FAILURE;
    }

    if(ehdr->e_ident[EI_CLASS] != ELFCLASS32) {
        jinue_error("error: unsupported init program ELF binary: bad file class");
        return EXIT_FAILURE;
    }

    if(ehdr->e_ident[EI_DATA] != ELFDATA2LSB) {
        jinue_error("error: unsupported init program ELF binary: bad endianess");
        return EXIT_FAILURE;
    }

    if(ehdr->e_version != 1 || ehdr->e_ident[EI_VERSION] != 1) {
        jinue_error("error: unsupported init program ELF binary: not version 1");
        return EXIT_FAILURE;
    }

    if(ehdr->e_machine != EM_386) {
        jinue_error("error: unsupported init program ELF binary: architecture (not x86)");
        return EXIT_FAILURE;
    }

    if(ehdr->e_flags != 0) {
        jinue_error("error: unsupported init program ELF binary: flags");
        return EXIT_FAILURE;
    }

    if(ehdr->e_type != ET_EXEC) {
        jinue_error("error: unsupported init program ELF binary: not an executable");
        return EXIT_FAILURE;
    }

    if(ehdr->e_phoff == 0 || ehdr->e_phnum == 0) {
        jinue_error("error: unsupported init program ELF binary: no program headers");
        return EXIT_FAILURE;
    }

    if(ehdr->e_entry == 0) {
        jinue_error("error: unsupported init program ELF binary: no entry point");
        return EXIT_FAILURE;
    }

    if(ehdr->e_phentsize != sizeof(Elf32_Phdr)) {
        jinue_error("error: unsupported init program ELF binary: program header size");
        return EXIT_FAILURE;
    }

    if(size < ehdr->e_phoff + ehdr->e_phnum * ehdr->e_phentsize) {
        jinue_error("error: invalid init program ELF binary: program headers past end of file");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static int map_flags(Elf32_Word pflags) {
    /* set flags */
    int flags = 0;

    if(pflags & PF_R) {
        flags |= JINUE_PROT_READ;
    }

    if(pflags & PF_W) {
        flags |= JINUE_PROT_WRITE;
    }
    else if(pflags & PF_X) {
        flags |= JINUE_PROT_EXEC;
    }

    return flags;
}

static int load_segments(elf_info_t *elf_info, int fd, const Elf32_Ehdr *ehdr) {
    /* program header table */
    Elf32_Phdr *phdr        = (Elf32_Phdr *)((char *)ehdr + ehdr->e_phoff);

    elf_info->at_phdr       = NULL; /* set below */
    elf_info->at_phnum      = ehdr->e_phnum;
    elf_info->at_phent      = ehdr->e_phentsize;
    elf_info->entry         = (void *)ehdr->e_entry;

    for(int idx = 0; idx < ehdr->e_phnum; ++idx) {
       if(phdr[idx].p_type != PT_LOAD) {
           continue;
       }

       /* TODO implement this */
    }

    if(elf_info->at_phdr == NULL) {
        jinue_error("error: program headers address (AT_PHDR) could not be determined");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int load_elf(int fd, const jinue_dirent_t *dirent) {
    const Elf32_Ehdr *ehdr = dirent->file;

    int status = check_elf_header(ehdr, dirent->size);

    elf_info_t elf_info;

    if(status == EXIT_SUCCESS) {
        status = load_segments(&elf_info, fd, ehdr);
    }

    /* TODO implement stack initialization, etc. */

    return status;
}
