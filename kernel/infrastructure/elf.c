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

#include <jinue/shared/asm/syscall.h>
#include <jinue/shared/vm.h>
#include <kernel/domain/alloc/page_alloc.h>
#include <kernel/domain/alloc/vmalloc.h>
#include <kernel/domain/entities/descriptor.h>
#include <kernel/domain/entities/object.h>
#include <kernel/domain/entities/process.h>
#include <kernel/domain/services/cmdline.h>
#include <kernel/domain/services/logging.h>
#include <kernel/domain/services/panic.h>
#include <kernel/infrastructure/elf.h>
#include <kernel/machine/asm/machine.h>
#include <kernel/machine/auxv.h>
#include <kernel/machine/vm.h>
#include <kernel/utils/utils.h>
#include <assert.h>
#include <string.h>

/**
 * Print validation error message and return false
 *
 * Utility function used by elf_check(). Prints an error message. Returns false
 * so it can be used by the caller to print the message and return false in
 * one statement.
 *
 * @param elf ELF header, which is at the very beginning of the ELF binary
 * @return always false
 *
 * */
static bool check_failed(const char *message) {
    error("Invalid ELF binary: %s", message);
    return false;
}

/**
 * Check the validity of an ELF binary
 *
 * @param ehdr ELF header, which is at the very beginning of the ELF binary
 * @return true if ELF binary is valid, false otherwise
 *
 * */
bool elf_check(Elf32_Ehdr *ehdr) {
    /* check: valid ELF binary magic number */
    if(     ehdr->e_ident[EI_MAG0] != ELF_MAGIC0 ||
            ehdr->e_ident[EI_MAG1] != ELF_MAGIC1 ||
            ehdr->e_ident[EI_MAG2] != ELF_MAGIC2 ||
            ehdr->e_ident[EI_MAG3] != ELF_MAGIC3 ) {
        return check_failed("not an ELF binary (ELF identification/magic check)");
    }
    
    /* check: 32-bit objects */
    if(ehdr->e_ident[EI_CLASS] != ELFCLASS32) {
        return check_failed("bad file class");
    }
    
    /* check: endianess */
    if(ehdr->e_ident[EI_DATA] != ELFDATA2LSB) {
        return check_failed("bad endianess");
    }
    
    /* check: version */
    if(ehdr->e_version != 1 || ehdr->e_ident[EI_VERSION] != 1) {
        return check_failed("not ELF version 1");
    }
    
    /* check: machine */
    if(ehdr->e_machine != EM_386) {
        return check_failed("not for x86 architecture");
    }
    
    /* check: the 32-bit Intel architecture defines no flags */
    if(ehdr->e_flags != 0) {
        return check_failed("invalid flags");
    }
    
    /* check: file type is executable */
    if(ehdr->e_type != ET_EXEC) {
        return check_failed("not an an executable");
    }
    
    /* check: must have a program header */
    if(ehdr->e_phoff == 0 || ehdr->e_phnum == 0) {
        return check_failed("no program headers");
    }
    
    /* check: must have an entry point */
    if(ehdr->e_entry == 0) {
        return check_failed("no entry point");
    }
    
    /* check: program header entry size */
    if(ehdr->e_phentsize != sizeof(Elf32_Phdr)) {
        return check_failed("unsupported program header size");
    }

    return true;
}

/**
 * Map a user space page
 *
 * This function leads to a kernel panic if the mapping fails because a
 * translation table could not be allocated.
 *
 * @param process address space for the mapping
 * @param vadds virtual address of page
 * @param padds physical address of page frame
 * @param flags mapping flags
 *
 * */
static void checked_map_userspace_page(
        process_t       *process,
        void            *vaddr,
        user_paddr_t     paddr,
        int              flags) {

    /* TODO check user space pointers
     *
     * This should not be necessary because we control the user loader binary,
     * but let's check anyway in case the build process breaks somehow. */
    if(! machine_map_userspace(process, vaddr, PAGE_SIZE, paddr, flags)) {
        panic("Page table allocation error when loading ELF file");
    }
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
 * Get program header for executable segment
 *
 * This function leads to a kernel panic if the mapping fails because a
 * translation table could not be allocated.
 *
 * @param elf ELF header
 * @return program header if found, NULL otherwise
 *
 * */
const Elf32_Phdr *elf_executable_program_header(const Elf32_Ehdr *ehdr) {
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
 * @param n number of entries to initialize
 * @param str pointer to the first string
 *
 * */
static void initialize_string_array(const char *array[], size_t n, const char *str) {
    for(int idx = 0; idx < n; ++idx) {
        /* Write address of current string. */
        array[idx] = str;

        /* skip to end of string */
        while(*str != '\0') {
            ++str;
        }

        /* skip terminator to get to next string */
        ++str;
    }
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
static addr_t get_at_phdr(const Elf32_Ehdr *ehdr) {
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
            return (addr_t)phdr->p_vaddr + ehdr->e_phoff - phdr->p_offset;
        }
    }

    panic("Program headers address (AT_PHDR) could not be determined");
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
 * This function is a wrapper around jinue_mclone() with debug logging if
 * requested with the DEBUG_LOADER_VERBOSE_MCLONE environment variable.
 * 
 * src_addr, dest_addr and length must be aligned on a page boundary.
 * 
 * @param elf_info ELF information structure (output)
 * @param ehdr ELF header
 * @param process process in which to load the binary
 *
 * */
static void load_segments(
        elf_info_t          *elf_info,
        process_t           *process,
        const Elf32_Ehdr    *ehdr) {

    const Elf32_Phdr *phdrs = program_header_table(ehdr);
    elf_info->at_phdr       = get_at_phdr(ehdr);
    elf_info->at_phnum      = ehdr->e_phnum;
    elf_info->at_phent      = ehdr->e_phentsize;
    elf_info->process       = process;
    elf_info->entry         = (void *)ehdr->e_entry;

    for(unsigned int idx = 0; idx < ehdr->e_phnum; ++idx) {
        const Elf32_Phdr *phdr = &phdrs[idx];

       if(phdr->p_type != PT_LOAD) {
           continue;
       }

        /* check that the segment is not in the region reserved for kernel use */
        if(! check_userspace_buffer((void *)phdr->p_vaddr, phdr->p_memsz)) {
            panic("user space loader memory layout -- address of segment too low");
        }

        /* set start and end addresses for mapping and copying */
        char *file_ptr  = (char *)ehdr + phdr->p_offset;
        char *vptr      = (char *)phdr->p_vaddr;
        char *vend      = vptr  + phdr->p_memsz;  /* limit for padding */
        char *vfend     = vptr  + phdr->p_filesz; /* limit for copy */

        /* align on page boundaries, be inclusive, note that vfend is not aligned */
        file_ptr        = ALIGN_START_PTR(file_ptr, PAGE_SIZE);
        vptr            = ALIGN_START_PTR(vptr,     PAGE_SIZE);
        vend            = ALIGN_END_PTR(vend,       PAGE_SIZE);

        bool is_writable    = !!(phdr->p_flags & PF_W);
        bool needs_padding  = (phdr->p_filesz != phdr->p_memsz);

        if(! (is_writable || needs_padding)) {
            /* Since the segment has to be mapped read only and does not require
             * padding, we can just map the original pages. */
            while(vptr < vend) {
                checked_map_userspace_page(
                        process,
                        vptr,
                        machine_lookup_kernel_paddr(file_ptr),
                        map_flags(phdr->p_flags));

                vptr     += PAGE_SIZE;
                file_ptr += PAGE_SIZE;
           }
        }
        else {
            /* Segment is writable and/or needs padding. We need to allocate new
             * pages for this segment. */
            while(vptr < vend) {
                /* start of this page and next page */
                char *vnext = vptr + PAGE_SIZE;

                /* allocate and map the new page */
                void *page = page_alloc();

                checked_map_userspace_page(
                        process,
                        vptr,
                        machine_lookup_kernel_paddr(page),
                        map_flags(phdr->p_flags));

                /* TODO transfer page ownership to userspace */

                /* copy */
                char *stop;

                if(vnext > vfend) {
                    stop = vfend;
                }
                else {
                    stop = vnext;
                }

                while(vptr < stop) {
                    *(vptr++) = *(file_ptr++);
                }

                /* pad */
                while(vptr < vnext) {
                    *(vptr++) = 0;
                }
           }
       }
    }
}

/**
 * Allocate the stack for ELF binary
 *
 * @param elf_info ELF information structure (output)
*
 * */
static void allocate_stack(elf_info_t *elf_info) {
    /** TODO: check for overlap of stack with loaded segments */

    for(addr_t vpage = (addr_t)JINUE_STACK_START; vpage < (addr_t)JINUE_STACK_BASE; vpage += PAGE_SIZE) {
        void *page = page_alloc();

        /* This newly allocated page may have data left from a previous boot
         * which may contain sensitive information. Let's clear it. */
        clear_page(page);

        checked_map_userspace_page(
                elf_info->process,
                vpage,
                machine_lookup_kernel_paddr(page),
                JINUE_PROT_READ | JINUE_PROT_WRITE);

        /* TODO transfer page ownership to userspace */
    }
}

/**
 * Initialize stack for ELF binary
 *
 * Initializes the command line arguments, the environment variables and the
 * auxiliary vectors.
 *
 * @param elf_info ELF information structure (output)
 * @param cmdline full kernel command line
 *
 * */
static void initialize_stack(
        elf_info_t *elf_info,
        const char *cmdline,
        const char *argv0) {

    uintptr_t *sp           = (uintptr_t *)(JINUE_STACK_BASE - JINUE_RESERVED_STACK_SIZE);
    elf_info->stack_addr    = sp;

    /* We add 1 because argv[0] is the program name, which is not on the kernel
     * command line. */
    size_t argc = cmdline_count_arguments(cmdline) + 1;
    *(sp++) = argc;

    /* Reserve space for argv and remember where we are. We will fill in the
     * pointers later. We add 1 to argc for the terminating NULL entry. */
    const char **argv = (const char **)sp;
    argv[argc] = NULL;
    sp += argc + 1;

    /* Reserve space for envp. Again, we will fill in the pointer values later.
     * We add 1 to nenv for the terminating NULL entry. */
    size_t nenv = cmdline_count_environ(cmdline);
    const char **envp = (const char **)sp;
    envp[nenv] = NULL;
    sp += nenv + 1;

    /* Auxiliary vectors */
    Elf32_auxv_t *auxvp = (Elf32_auxv_t *)sp;
    sp = (uintptr_t *)(auxvp + 8);

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
    auxvp[5].a_un.a_val = JINUE_STACK_BASE;

    auxvp[6].a_type     = JINUE_AT_HOWSYSCALL;
    auxvp[6].a_un.a_val = machine_at_howsyscall();

    auxvp[7].a_type     = JINUE_AT_NULL;
    auxvp[7].a_un.a_val = 0;

    /* Write arguments and environment variables (i.e. the actual strings). */
    char *const args = (char *)sp;

    strcpy(args, argv0);

    char *const arg1 = args + strlen(argv0) + 1;

    char *const envs = cmdline_write_arguments(arg1, cmdline);

    cmdline_write_environ(envs, cmdline);

    /* Fill in content of pointer arrays argv and envp. */
    initialize_string_array(argv, argc, args);
    initialize_string_array(envp, nenv, envs);
}

/**
 * Initialize descriptors for user space loader
 *
 * This function initializes a single descriptor which references the process
 * itself (JINUE_SELF_PROCESS_DESCRIPTOR).
 * 
 * @param process process in which the ELF binary is loaded
 *
 * */
static void initialize_descriptors(process_t *process) {
    descriptor_t *desc;
    (void)dereference_unused_descriptor(&desc, process, JINUE_SELF_PROCESS_DESCRIPTOR);

    desc->object = &process->header;
    desc->flags  = DESCRIPTOR_FLAG_IN_USE;
    desc->cookie = 0;

    open_object(&process->header, desc);
}

/**
 * Load ELF binary
 *
 * This function is intended to be used to load the user space loader binary,
 * not arbitrary user binaries. It loads the loadable segments, sets up the
 * stack and fills an ELF information structure with information about the
 * binary.
 *
 * @param thread_params initial thread parameters (output)
 * @param process process in which to load the ELF binary
 * @param exec_file executable ELF file
 * @param argv0 name of binary
 * @param cmdline full kernel command line
 *
 * */
void machine_load_exec(
        thread_params_t     *thread_params,
        process_t           *process,
        const exec_file_t   *exec_file,
        const char          *argv0,
        const char          *cmdline) {
    
    Elf32_Ehdr *ehdr = exec_file->start;

    if(! elf_check(ehdr)) {
        panic("ELF binary is invalid");
    }

    elf_info_t elf_info;
    load_segments(&elf_info, process, ehdr);

    allocate_stack(&elf_info);

    initialize_stack(&elf_info, cmdline, argv0);
    
    initialize_descriptors(process);

    info("ELF binary loaded.");

    thread_params->entry        = elf_info.entry;
    thread_params->stack_addr   = elf_info.stack_addr;
}

/**
 * Get pointer to ELF file as a bytes array
 *
 * @param ehdr ELF header of ELF binary
 * @return start of ELF file bytes
 *
 * */
static const char *elf_file_bytes(const Elf32_Ehdr *ehdr) {
    return (const char *)ehdr;
}

/**
 * Get an ELF section header by index
 * 
 * No bound check is performed. It is the caller's responsibility to ensure
 * 0 <= index < ehdr->e_shnum.
 *
 * @param ehdr ELF header of ELF binary
 * @param index index of the section header
 * @return pointer on section header
 *
 * */
static const Elf32_Shdr *elf_get_section_header(const Elf32_Ehdr *ehdr, int index) {
    const char *elf_file        = elf_file_bytes(ehdr);
    const char *section_table   = &elf_file[ehdr->e_shoff];
    
    return (const Elf32_Shdr *)&section_table[index * ehdr->e_shentsize];
}

/**
 * Find an ELF section header by type
 *
 * If multiple sections of the same type are present, the first instance is
 * returned.
 *
 * @param ehdr ELF header of ELF binary
 * @param type type of symbol
 * @return pointer on section header if found, NULL otherwise
 *
 * */
static const Elf32_Shdr *elf_find_section_header_by_type(
        const Elf32_Ehdr    *ehdr,
        Elf32_Word           type) {

    for(int idx = 0; idx < ehdr->e_shnum; ++idx) {
        const Elf32_Shdr *section_header = elf_get_section_header(ehdr, idx);

        if(section_header->sh_type == type) {
            return section_header;
        }
    }

    return NULL;
}

/**
 * Find the section header for the symbol table
 *
 * @param ehdr ELF header of ELF binary
 * @return pointer on section header if found, NULL otherwise
 *
 * */
static const Elf32_Shdr *elf_find_symtab_section_header(const Elf32_Ehdr *ehdr) {
    return elf_find_section_header_by_type(ehdr, SHT_SYMTAB);
}

/**
 * Get the binary data of a section
 *
 * @param ehdr ELF header of ELF binary
 * @param section_header ELF section header
 * @return symbol name as a NUL-terminated string
 *
 * */
static const char *elf_section_data(const Elf32_Ehdr *ehdr, const Elf32_Shdr *section_header) {
    const char *elf_file = elf_file_bytes(ehdr);
    return &elf_file[section_header->sh_offset];
}

/**
 * Look up the name of a symbol
 *
 * @param ehdr ELF header of ELF binary
 * @param symbol_header ELF symbol header
 * @return symbol name as a NUL-terminated string
 *
 * */
const char *elf_symbol_name(const Elf32_Ehdr *ehdr, const Elf32_Sym *symbol_header) {
    /* Here, we can safely assume the symbol table exists because the symbol
     * header passed as argument had to be looked up there. */
    const Elf32_Shdr *string_section_header = elf_get_section_header(
            ehdr,
            elf_find_symtab_section_header(ehdr)->sh_link);

    const char *string_table = elf_section_data(ehdr, string_section_header);

    return &string_table[symbol_header->st_name];
}

/**
 * Look up a symbol in the ELF binary's symbol table by address and type
 *
 * Input is an address and a symbol type. The result contains the start address
 * and length of the symbol.
 *
 * @param ehdr ELF header of ELF binary
 * @param addr address to look up
 * @param type type of the symbol
 * @return symbol header if found, NULL otherwise
 *
 * */
static const Elf32_Sym *find_symbol_by_address_and_type(
        const Elf32_Ehdr    *ehdr,
        Elf32_Addr           addr,
        int                  type) {

    const Elf32_Shdr *section_header = elf_find_symtab_section_header(ehdr);

    if(section_header == NULL) {
        /* no symbol table */
        return NULL;
    }

    const char *symbols_table = elf_section_data(ehdr, section_header);
    
    for(int offset = 0; offset < section_header->sh_size; offset += section_header->sh_entsize) {
        const Elf32_Sym *symbol_header = (const Elf32_Sym *)&symbols_table[offset];
        
        if(ELF32_ST_TYPE(symbol_header->st_info) != type) {
            continue;
        }

        Elf32_Addr lookup_addr  = (Elf32_Addr)addr;
        Elf32_Addr start        = symbol_header->st_value;
        Elf32_Addr end          = start + symbol_header->st_size;

        if(lookup_addr >= start && lookup_addr < end) {
            return symbol_header;
        }
    }
    
    /* not found */
    return NULL;
}

/**
 * Look up a function symbol in the ELF binary's symbol table by address
 *
 * Input is an address and a symbol type. The result contains the start address
 * and length of the symbol.
 *
 * @param ehdr ELF header of ELF binary
 * @param addr address to look up
 * @return symbol header if found, NULL otherwise
 *
 * */
const Elf32_Sym *elf_find_function_symbol_by_address(const Elf32_Ehdr *ehdr, Elf32_Addr addr) {
    return find_symbol_by_address_and_type(ehdr, addr, STT_FUNCTION);
}
