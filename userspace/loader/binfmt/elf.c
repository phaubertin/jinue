/*
 * Copyright (C) 2023-2024 Philippe Aubertin.
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
#include <jinue/utils.h>
#include <sys/auxv.h>
#include <sys/elf.h>
#include <sys/mman.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "../core/mappings.h"
#include "../core/meminfo.h"
/* TODO remove this */
#include "../descriptors.h"
#include "../utils.h"
#include "elf.h"

typedef struct {
    void    (*entry)(void);
    void    *stack_addr;
    void    *at_phdr;
    int      at_phent;
    int      at_phnum;
} elf_info_t;

/**
 * Validate the ELF header
 *
 * @param ehdr ELF file header
 * @param size total size of the ELF binary in bytes
 * @return EXIT_SUCCESS if header is valid, EXIT_FAILURE otherwise
 *
 * */
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

/**
 * Get program header table
 *
 * @param ehdr ELF file header
 * @return program header table
 *
 * */
static const Elf32_Phdr *program_header_table(const Elf32_Ehdr *ehdr) {
    return (Elf32_Phdr *)((char *)ehdr + ehdr->e_phoff);
}

/**
 * Get value of AT_PHDR axiliary vector
 *
 * AT_PHDR is the address of the program headers table in the user address space.
 *
 * @param ehdr ELF file header
 * @return value of AT_PHDR
 *
 * */
static void *get_at_phdr(const Elf32_Ehdr *ehdr) {
    const Elf32_Phdr *phdrs     = program_header_table(ehdr);
    Elf32_Off phdr_filestart    = ehdr->e_phoff;
    Elf32_Off phdr_fileend      = ehdr->e_phoff + ehdr->e_phnum * ehdr->e_phentsize;

    for(unsigned int idx = 0; idx < ehdr->e_phnum; ++idx) {
        const Elf32_Phdr *phdr = &phdrs[idx];

        if(phdr->p_type != PT_LOAD || (phdr->p_flags & PF_W)) {
            continue;
        }

        Elf32_Off p_filestart   = phdr->p_offset;
        Elf32_Off p_fileend     = phdr->p_offset + phdr->p_filesz;

        if(p_filestart <= phdr_filestart && phdr_fileend <= p_fileend) {
            /* We found the segment that completely contains the program headers table. */
            return (char *)phdr->p_vaddr + ehdr->e_phoff - phdr->p_offset;
        }
    }

    jinue_error("error: program headers address (AT_PHDR) could not be determined");
    return NULL;
}

/**
 * Map the protection flags
 * 
 * Maps the protection flags in a program headers' p_flags member to the
 * JINUE_PROT_READ, JINUE_PROT_WRITE and/or JINUE_PROT_EXEC protection
 * flags.
 * 
 * @param p_flags flags
 * @return start of stack in loader address space
 *
 * */
static int map_flags(Elf32_Word p_flags) {
    /* set flags */
    int flags = 0;

    if(p_flags & PF_R) {
        flags |= JINUE_PROT_READ;
    }

    if(p_flags & PF_W) {
        flags |= JINUE_PROT_WRITE;
    }
    else if(p_flags & PF_X) {
        flags |= JINUE_PROT_EXEC;
    }

    return flags;
}

/**
 * Load the loadable (PT_LOAD) segments from the ELF binary
 * 
 * @param elf_info ELF information structure (output)
 * @param fd descriptor of the process in which to load the binary
 * @param ehdr ELF header
 * @return EXIT_SUCCESS on success, EXIT_FAILURE on failure
 *
 * */
static int load_segments(elf_info_t *elf_info, const file_t *exec_file) {
    const Elf32_Ehdr *ehdr = exec_file->contents;

    /* program header table */
    const Elf32_Phdr *phdrs = program_header_table(ehdr);
    elf_info->at_phdr       = get_at_phdr(ehdr);

    if(elf_info->at_phdr == NULL) {
        return EXIT_FAILURE;
    }

    elf_info->at_phnum      = ehdr->e_phnum;
    elf_info->at_phent      = ehdr->e_phentsize;
    elf_info->entry         = (void (*)(void))ehdr->e_entry;

    for(int idx = 0; idx < ehdr->e_phnum; ++idx) {
        const Elf32_Phdr *phdr = &phdrs[idx];

        if(phdr->p_type != PT_LOAD) {
            continue;
        }

        const uintptr_t diff    = phdr->p_vaddr % JINUE_PAGE_SIZE;
        const char *vaddr       = (char *)phdr->p_vaddr - diff;
        const size_t memsize    = (phdr->p_memsz + diff + JINUE_PAGE_SIZE - 1) & ~JINUE_PAGE_MASK;

        bool is_writable        = !!(phdr->p_flags & PF_W);
        bool needs_padding      = (phdr->p_filesz != phdr->p_memsz);

        if(! (is_writable || needs_padding)) {
            int status = map_file(
                (void *)vaddr,
                memsize,
                exec_file->segment_index,
                phdr->p_offset - diff,
                map_flags(phdr->p_flags)
            );

            if(status < 0) {
                return EXIT_FAILURE;
            }
        } else {
            char *segment = map_anonymous((void *)vaddr, memsize, map_flags(phdr->p_flags));

            if(segment == NULL) {
                return EXIT_FAILURE;
            }

            /* Copy and pad segment content. */
            memset(segment, 0, diff);

            memcpy(
                segment + diff,
                (char *)ehdr + phdr->p_offset,
                phdr->p_filesz
            );

            memset(segment + diff + phdr->p_filesz, 0, memsize - phdr->p_filesz - diff);
        }
    }

    return EXIT_SUCCESS;
}

/**
 * Allocate and map the stack
 *
 * @return start of stack in loader address space
 *
 * */
static void *allocate_stack(void) {
    /** TODO: check for overlap of stack with loaded segments */

    char *stack = map_anonymous(
        (void *)JINUE_STACK_START,
        JINUE_STACK_SIZE,
        PROT_READ | PROT_WRITE
    );

    if(stack == NULL) {
        return NULL;
    }

    /* This newly allocated page may have data left from a previous boot
     * which may contain sensitive information. Let's clear it. */
    memset(stack, 0, JINUE_STACK_SIZE);

    return stack;
}

extern char **environ;

/**
 * Count the environment variables
 *
 * @return number of environment variables
 *
 * */
size_t count_environ(void) {
    int n = 0;

    while(environ[n] != NULL) {
        ++n;
    }

    return n;
}

/**
 * Write the command line argument strings
 * 
 * The loader's own argc and argv should be passed as the argc and argv
 * parameters. This function takes care of substituting argv[0] with the
 * file name from the ELF binary directory entry.
 *
 * @param dest write destination
 * @param dirent directory entry for the ELF binary
 * @param argc number of command line arguments
 * @param argv command line arguments
 * @return address of first byte following the last terminator
 * 
 * */
char *write_cmdline_arguments(char *dest, const file_t *exec_file, int argc, char *argv[]) {
    strcpy(dest, exec_file->name);
    dest += strlen(exec_file->name) + 1;

    for(int idx = 1; idx < argc; ++ idx) {
        strcpy(dest, argv[idx]);
        dest += strlen(argv[idx]) + 1;
    }

    return dest;
}

/**
 * Write the environment variable strings
 *
 * @param dest write destination
 * 
 * */
void write_environ(char *dest) {
    for(int idx = 0; environ[idx] != NULL; ++idx) {
        strcpy(dest, environ[idx]);
        dest += strlen(environ[idx]) + 1;
    }
}

/**
 * Initialize the arguments (argv) and environment variables string arrays
 *
 * This function is intended to initialize the string arrays for the command
 * line arguments (argv) and environment variables. It does not initialize the
 * terminating NULL entry, which needs to be intialized separately.
 *
 * This function intializes a fixed number of entries and assumes the
 * NUL-terminated strings are concatenated.
 *
 * @param array array to initialize
 * @param local pointer to the first string in this process' address space
 * @param remote pointer to the first string for the process where the ELF binary is loaded
 * @param n number of entries to initialize
 *
 * */
static void initialize_string_array(
        const char  *array[],
        size_t       n,
        const char  *local,
        const char  *remote) {

    int offset = 0;

    for(int idx = 0; idx < n; ++idx) {
        /* Write address of current string. */
        array[idx] = &remote[offset];

        /* skip to end of string */
        while(local[offset] != '\0') {
            ++offset;
        }

        /* skip terminator to get to next string */
        ++offset;
    }
}

/**
 * Initialize the stack
 * 
 * Initializes the command line arguments, the environment variables and the
 * auxiliary vectors.
 * 
 * The loader's own argc and argv should be passed as the argc and argv
 * parameters. This function takes care of substituting argv[0] with the
 * file name from the ELF binary directory entry.
 *
 * @param stack allocated stack segment
 * @param elf_info ELF information structure (in and out)
 * @param dirent directory entry for the ELF binary
 * @param argc number of command line arguments
 * @param argv command line arguments
 *
 * */
static void initialize_stack(
        void            *stack,
        elf_info_t      *elf_info,
        const file_t    *exec_file,
        int              argc,
        char            *argv[]) {

    char *local     = (char *)stack + JINUE_STACK_SIZE - JINUE_RESERVED_STACK_SIZE;
    char *remote    = (char *)(JINUE_STACK_BASE - JINUE_RESERVED_STACK_SIZE);
    int index       = 0;

    elf_info->stack_addr = remote;

    uintptr_t *wlocal = (uintptr_t *)local;
    wlocal[index++] = argc;

    /* Reserve space for argv and remember where we are. We will fill in the
     * pointers later. We add 1 to argc for the terminating NULL entry. */
    const char **stack_argv = (const char **)&wlocal[index];
    stack_argv[argc] = NULL;
    index += argc + 1;

    /* Reserve space for envp. Again, we will fill in the pointer values later.
     * We add 1 to nenv for the terminating NULL entry. */
    size_t nenv = count_environ();
    const char **envp = (const char **)&wlocal[index];
    envp[nenv] = NULL;
    index += nenv + 1;

    /* Auxiliary vectors */
    Elf32_auxv_t *auxvp = (Elf32_auxv_t *)&wlocal[index];
    index += 8 * sizeof(auxvp[0]) / sizeof(wlocal[0]);

    auxvp[0].a_type     = JINUE_AT_PHDR;
    auxvp[0].a_un.a_val = (uint32_t)elf_info->at_phdr;

    auxvp[1].a_type     = JINUE_AT_PHENT;
    auxvp[1].a_un.a_val = (uint32_t)elf_info->at_phent;

    auxvp[2].a_type     = JINUE_AT_PHNUM;
    auxvp[2].a_un.a_val = (uint32_t)elf_info->at_phnum;

    auxvp[3].a_type     = JINUE_AT_PAGESZ;
    auxvp[3].a_un.a_val = JINUE_PAGE_SIZE;

    auxvp[4].a_type     = JINUE_AT_ENTRY;
    auxvp[4].a_un.a_val = (uint32_t)elf_info->entry;

    auxvp[5].a_type     = JINUE_AT_STACKBASE;
    auxvp[5].a_un.a_val = JINUE_STACK_BASE;

    auxvp[6].a_type     = JINUE_AT_HOWSYSCALL;
    auxvp[6].a_un.a_val = getauxval(JINUE_AT_HOWSYSCALL);

    auxvp[7].a_type     = JINUE_AT_NULL;
    auxvp[7].a_un.a_val = 0;

    char *const args = (char *)&wlocal[index];

    char *const envs = write_cmdline_arguments(args, exec_file, argc, argv);
    
    write_environ(envs);

    initialize_string_array(stack_argv, argc, args, remote + (args - local));
    initialize_string_array(envp, nenv, envs, remote + (envs - local));
}

/**
 * Load ELF binary
 *
 * This function loads the loadable segments of an ELF binary, sets up the
 * stack and fills an ELF information structure with information about the
 * binary.
 * 
 * The loader's own argc and argv should be passed as the argc and argv
 * parameters. This function takes care of substituting argv[0] with the
 * file name from the ELF binary directory entry.
 *
 * @param elf_info ELF information structure (output)
 * @param fd descriptor of the process in which to load the binary
 * @param dirent directory entry for the ELF binary
 * @param argc number of command line arguments
 * @param argv command line arguments
 *
 * */
int load_elf(
        thread_params_t *thread_params,
        const file_t    *exec_file,
        int              argc,
        char            *argv[]) {

    const Elf32_Ehdr *ehdr = exec_file->contents;

    int status = check_elf_header(ehdr, exec_file->size);

    if(status != EXIT_SUCCESS) {
        return status;
    }

    elf_info_t elf_info;
    status = load_segments(&elf_info, exec_file);

    if(status != EXIT_SUCCESS) {
        return status;
    }

    void *stack = allocate_stack();
    
    if(stack == NULL) {
        return EXIT_FAILURE;
    }

    initialize_stack(stack, &elf_info, exec_file, argc, argv);

    thread_params->entry        = elf_info.entry;
    thread_params->stack_addr   = elf_info.stack_addr;

    return EXIT_SUCCESS;
}
