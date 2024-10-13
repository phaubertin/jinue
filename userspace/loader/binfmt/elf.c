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
#include <jinue/util.h>
#include <sys/auxv.h>
#include <sys/elf.h>
#include <sys/mman.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "../util.h"
#include "elf.h"

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

    jinue_error("Program headers address (AT_PHDR) could not be determined");
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
 * String representation of protection flags
 * 
 * @param prot protection flags
 * @return string representation
 *
 * */
static const char *prot_str(int prot) {
    static char buffer[4];
    buffer[0] = (prot & PROT_READ)  ? 'r' : '-';
    buffer[1] = (prot & PROT_WRITE) ? 'w' : '-';
    buffer[2] = (prot & PROT_EXEC)  ? 'x' : '-';
    buffer[3] = '\0';
    return buffer;
}

/**
 * Clone a segment mapping from this process to the one where the ELF binary is loaded
 * 
 * This function is a wrapper around jinue_mclone() with debug logging if
 * requested with the DEBUG_LOADER_VERBOSE_MCLONE environment variable.
 * 
 * src_addr, dest_addr and length must be aligned on a page boundary.
 * 
 * @param fd descriptor of the process in which too load the binary
 * @param src_addr segment start address in this process
 * @param dest_addr segment start address in the other process
 * @param length length of segment in bytes
 * @param prot protection flags
 * @return start of stack in loader address space
 *
 * */
static int clone_mapping(
        int      fd,
        void    *src_addr,
        void    *dest_addr,
        size_t   length,
        int      prot) {
    
    if(bool_getenv("DEBUG_LOADER_VERBOSE_MCLONE")) {
        jinue_info(
            "jinue_mclone(%d, %d, %#p, %#p, %#x, %s, &errno)",
            JINUE_SELF_PROCESS_DESCRIPTOR,
            fd,
            src_addr,
            dest_addr,
            length,
            prot_str(prot)
        );
    }

    int status = jinue_mclone(
        JINUE_SELF_PROCESS_DESCRIPTOR,
        fd,
        src_addr,
        dest_addr,
        length,
        prot,
        &errno
    );

    if(status != 0) {
        jinue_error("jinue_mclone() failed: %s", strerror(errno));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/**
 * Load the loadable (PT_LOAD) segments from the ELF binary
 * 
 * This function is a wrapper around jinue_mclone() with debug logging if
 * requested with the DEBUG_LOADER_VERBOSE_MCLONE environment variable.
 * 
 * src_addr, dest_addr and length must be aligned on a page boundary.
 * 
 * @param elf_info ELF information structure (output)
 * @param fd descriptor of the process in which to load the binary
 * @param ehdr ELF header
 * @return EXIT_SUCCESS on success, EXIT_FAILURE on failure
 *
 * */
static int load_segments(elf_info_t *elf_info, int fd, const Elf32_Ehdr *ehdr) {
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

        const uintptr_t diff    = phdr->p_vaddr % PAGE_SIZE;
        const char *vaddr       = (char *)phdr->p_vaddr - diff;
        const size_t memsize    = (phdr->p_memsz + diff + PAGE_SIZE - 1) & ~PAGE_MASK;

        bool is_writable        = !!(phdr->p_flags & PF_W);
        bool needs_padding      = (phdr->p_filesz != phdr->p_memsz);

        char *segment;

        if(! (is_writable || needs_padding)) {
            /* Since the segment has to be mapped read only and does not require
             * padding, we can just map the original pages. */
            segment = (char *)ehdr + phdr->p_offset - diff;
        } else {
            /* Copy and pad segment content */
            segment = mmap(
                NULL,
                memsize,
                PROT_READ | PROT_WRITE,
                MAP_SHARED | MAP_ANONYMOUS,
                -1,
                0
            );

            if(segment == MAP_FAILED) {
                jinue_error("mmap() failed: %s", strerror(errno));
                return EXIT_FAILURE;
            }

            memset(segment, 0, diff);

            memcpy(
                segment + diff,
                (char *)ehdr + phdr->p_offset,
                phdr->p_filesz
            );

            memset(segment + diff + phdr->p_filesz, 0, memsize - phdr->p_filesz - diff);
        }

        int status = clone_mapping(
            fd,
            segment,
            (void *)vaddr,
            memsize,
            map_flags(phdr->p_flags)
        );

        if(status != EXIT_SUCCESS) {
            jinue_error("jinue_mclone() failed: %s", strerror(errno));
            return status;
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
static void *allocate_stack(int fd) {
    /** TODO: check for overlap of stack with loaded segments */

    char *stack = mmap(
        NULL,
        STACK_SIZE,
        PROT_READ | PROT_WRITE,
        MAP_SHARED | MAP_ANONYMOUS,
        -1,
        0
    );

    if(stack == MAP_FAILED) {
        return MAP_FAILED;
    }

    /* This newly allocated page may have data left from a previous boot
     * which may contain sensitive information. Let's clear it. */
    memset(stack, 0, STACK_SIZE);

    int status = clone_mapping(
        fd,
        stack,
        (void *)STACK_START,
        STACK_SIZE,
        PROT_READ | PROT_WRITE
    );

    return (status == EXIT_SUCCESS) ? stack : MAP_FAILED;
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
char *write_cmdline_arguments(char *dest, const jinue_dirent_t *dirent, int argc, char *argv[]) {
    strcpy(dest, dirent->name);
    dest += strlen(dirent->name) + 1;

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
        void                    *stack,
        elf_info_t              *elf_info,
        const jinue_dirent_t    *dirent,
        int                      argc,
        char                    *argv[]) {

    char *local     = (char *)stack + STACK_SIZE - RESERVED_STACK_SIZE;
    char *remote    = (char *)(STACK_BASE - RESERVED_STACK_SIZE);
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
    auxvp[3].a_un.a_val = PAGE_SIZE;

    auxvp[4].a_type     = JINUE_AT_ENTRY;
    auxvp[4].a_un.a_val = (uint32_t)elf_info->entry;

    auxvp[5].a_type     = JINUE_AT_STACKBASE;
    auxvp[5].a_un.a_val = STACK_BASE;

    auxvp[6].a_type     = JINUE_AT_HOWSYSCALL;
    auxvp[6].a_un.a_val = getauxval(JINUE_AT_HOWSYSCALL);

    auxvp[7].a_type     = JINUE_AT_NULL;
    auxvp[7].a_un.a_val = 0;

    char *const args = (char *)&wlocal[index];

    char *const envs = write_cmdline_arguments(args, dirent, argc, argv);
    
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
 * @param fd descriptor of the process in which too load the binary
 * @param dirent directory entry for the ELF binary
 * @param argc number of command line arguments
 * @param argv command line arguments
 *
 * */
int load_elf(elf_info_t *elf_info, int fd, const jinue_dirent_t *dirent, int argc, char *argv[]) {
    const Elf32_Ehdr *ehdr = dirent->file;

    int status = check_elf_header(ehdr, dirent->size);

    if(status != EXIT_SUCCESS) {
        return status;
    }

    status = load_segments(elf_info, fd, ehdr);

    if(status != EXIT_SUCCESS) {
        return status;
    }

    void *stack = allocate_stack(fd);
    
    if(stack == MAP_FAILED) {
        return EXIT_FAILURE;
    }

    initialize_stack(stack, elf_info, dirent, argc, argv);

    return EXIT_SUCCESS;
}
