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

#include <kernel/i686/trap.h>
#include <kernel/i686/vm.h>
#include <kernel/boot.h>
#include <kernel/cmdline.h>
#include <kernel/elf.h>
#include <kernel/logging.h>
#include <kernel/page_alloc.h>
#include <kernel/panic.h>
#include <kernel/vmalloc.h>
#include <kernel/util.h>
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
 * @param elf ELF header, which is at the very beginning of the ELF binary
 * @return true if ELF binary is valid, false otherwise
 *
 * */
bool elf_check(Elf32_Ehdr *elf) {
    /* check: valid ELF binary magic number */
    if(     elf->e_ident[EI_MAG0] != ELF_MAGIC0 ||
            elf->e_ident[EI_MAG1] != ELF_MAGIC1 ||
            elf->e_ident[EI_MAG2] != ELF_MAGIC2 ||
            elf->e_ident[EI_MAG3] != ELF_MAGIC3 ) {
        return check_failed("not an ELF binary (ELF identification/magic check)");
    }
    
    /* check: 32-bit objects */
    if(elf->e_ident[EI_CLASS] != ELFCLASS32) {
        return check_failed("bad file class");
    }
    
    /* check: endianess */
    if(elf->e_ident[EI_DATA] != ELFDATA2LSB) {
        return check_failed("bad endianess");
    }
    
    /* check: version */
    if(elf->e_version != 1 || elf->e_ident[EI_VERSION] != 1) {
        return check_failed("not ELF version 1");
    }
    
    /* check: machine */
    if(elf->e_machine != EM_386) {
        return check_failed("not for x86 architecture");
    }
    
    /* check: the 32-bit Intel architecture defines no flags */
    if(elf->e_flags != 0) {
        return check_failed("invalid flags");
    }
    
    /* check: file type is executable */
    if(elf->e_type != ET_EXEC) {
        return check_failed("not an an executable");
    }
    
    /* check: must have a program header */
    if(elf->e_phoff == 0 || elf->e_phnum == 0) {
        return check_failed("no program headers");
    }
    
    /* check: must have an entry point */
    if(elf->e_entry == 0) {
        return check_failed("no entry point");
    }
    
    /* check: program header entry size */
    if(elf->e_phentsize != sizeof(Elf32_Phdr)) {
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
 * @param addr_space address space for mapping
 * @param vadds virtual address of page
 * @param padds physical address of page frame
 * @param flags mapping flags
 *
 * */
static void checked_map_userspace(
        addr_space_t    *addr_space,
        void            *vaddr,
        user_paddr_t     paddr,
        int              flags) {

    /* TODO check user space pointers
     *
     * This should not be necessary because we control the user loader binary,
     * but let's check anyway in case the build process breaks somehow. */
    if(! vm_map_userspace(addr_space, vaddr, paddr, flags)) {
        panic("Page table allocation error when loading ELF file");
    }
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
const Elf32_Phdr *elf_executable_program_header(const Elf32_Ehdr *elf) {
    /* get the program header table */
    Elf32_Phdr *phdr = (Elf32_Phdr *)((char *)elf + elf->e_phoff);

    for(int idx = 0; idx < elf->e_phnum; ++idx) {
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
 * Load ELF binary
 *
 * This function is intended to be used to load the user space loader binary,
 * not arbitrary user binaries. It loads the loadable segments, sets up the
 * stack and fills an ELF information structure with information about the
 * binary.
 *
 * @param info ELF information structure (output)
 * @param elf ELF file header
 * @param argv0 name of binary
 * @param cmdline full kernel command line
 * @param addr_space address space
 * @param boot_alloc boot-time page allocator
 *
 * */
void elf_load(
        elf_info_t      *elf_info,
        Elf32_Ehdr      *elf,
        const char      *argv0,
        const char      *cmdline,
        addr_space_t    *addr_space,
        boot_alloc_t    *boot_alloc) {
    
    /* get the program header table */
    Elf32_Phdr *phdr = (Elf32_Phdr *)((char *)elf + elf->e_phoff);
    
    elf_info->at_phdr       = (addr_t)phdr;
    elf_info->at_phnum      = elf->e_phnum;
    elf_info->at_phent      = elf->e_phentsize;
    elf_info->addr_space    = addr_space;
    elf_info->entry         = (void *)elf->e_entry;
    elf_info->argv0         = argv0;

    for(unsigned int idx = 0; idx < elf->e_phnum; ++idx) {
        if(phdr[idx].p_type != PT_LOAD) {
            continue;
        }
        
        /* check that the segment is not in the region reserved for kernel use */
        if(! check_userspace_buffer((void *)phdr[idx].p_vaddr, phdr[idx].p_memsz)) {
            panic("user space loader memory layout -- address of segment too low");
        }
        
        /* set start and end addresses for mapping and copying */
        char *file_ptr  = (char *)elf + phdr[idx].p_offset;
        char *vptr      = (char *)phdr[idx].p_vaddr;
        char *vend      = vptr  + phdr[idx].p_memsz;  /* limit for padding */
        char *vfend     = vptr  + phdr[idx].p_filesz; /* limit for copy */
                
        /* align on page boundaries, be inclusive, 
           note that vfend is not aligned        */
        file_ptr        = ALIGN_START_PTR(file_ptr, PAGE_SIZE);
        vptr            = ALIGN_START_PTR(vptr,     PAGE_SIZE);
        vend            = ALIGN_END_PTR(vend,       PAGE_SIZE);

        bool is_writable    = !!(phdr[idx].p_flags & PF_W);
        bool needs_padding  = (phdr[idx].p_filesz != phdr[idx].p_memsz);

        if(! (is_writable || needs_padding)) {
            /* Since the segment has to be mapped read only and does not require
             * padding, we can just map the original pages. */
            int flags = 0;

            if(phdr[idx].p_flags & PF_R) {
                flags |= VM_MAP_READ;
            }

            if(phdr[idx].p_flags & PF_X) {
                flags |= VM_MAP_EXEC;
            }

            while(vptr < vend) {
                checked_map_userspace(
                        addr_space,
                        vptr,
                        vm_lookup_kernel_paddr(file_ptr),
                        flags);

                vptr     += PAGE_SIZE;
                file_ptr += PAGE_SIZE;
            }
        }
        else {
            /* Segment is writable and/or needs padding. We need to allocate new
             * pages for this segment. */
            while(vptr < vend) {
                char *stop;

                /* start of this page and next page */
                char *vnext = vptr + PAGE_SIZE;
                
                /* set flags */
                int flags = 0;

                if(phdr[idx].p_flags & PF_R) {
                    flags |= VM_MAP_READ;
                }

                if(phdr[idx].p_flags & PF_W) {
                    flags |= VM_MAP_WRITE;
                }

                if(phdr[idx].p_flags & PF_X) {
                    flags |= VM_MAP_EXEC;
                }

                /* allocate and map the new page */
                void *page = page_alloc();
                checked_map_userspace(
                        addr_space,
                        vptr,
                        vm_lookup_kernel_paddr(page),
                        flags);

                /* TODO transfer page ownership to userspace */
                                
                /* copy */
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
    
    elf_allocate_stack(elf_info, boot_alloc);

    elf_initialize_stack(elf_info, cmdline);
    
    info("ELF binary loaded.");
}

/**
 * Allocate the stack for ELF binary
 *
 * @param info ELF information structure (output)
 * @param boot_alloc boot-time page allocator
 *
 * */
void elf_allocate_stack(elf_info_t *elf_info, boot_alloc_t *boot_alloc) {
    /** TODO: check for overlap of stack with loaded segments */

    for(addr_t vpage = (addr_t)STACK_START; vpage < (addr_t)STACK_BASE; vpage += PAGE_SIZE) {
        void *page = page_alloc();

        /* This newly allocated page may have data left from a previous boot
         * which may contain sensitive information. Let's clear it. */
        clear_page(page);

        checked_map_userspace(
                elf_info->addr_space,
                vpage,
                vm_lookup_kernel_paddr(page),
                VM_MAP_READ | VM_MAP_WRITE);

        /* TODO transfer page ownership to userspace */
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
 * @param str pointer to the first string
 * @param n number of entries to initialize
 *
 * */
static void initialize_string_array(const char *array[], const char *str, size_t n) {
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
 * Initialize stack for ELF binary
 *
 * Initializes the command line arguments, the environment variables and the
 * auxiliary vectors.
 *
 * @param info ELF information structure (output)
 * @param cmdline full kernel command line
 *
 * */
void elf_initialize_stack(elf_info_t *elf_info, const char *cmdline) {
    uintptr_t *sp       = (uintptr_t *)(STACK_BASE - RESERVED_STACK_SIZE);
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
    sp += (nenv + 1);

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
    auxvp[5].a_un.a_val = STACK_BASE;

    auxvp[6].a_type     = JINUE_AT_HOWSYSCALL;
    auxvp[6].a_un.a_val = syscall_implementation;

    auxvp[7].a_type     = JINUE_AT_NULL;
    auxvp[7].a_un.a_val = 0;

    /* Write arguments and environment variables (i.e. the actual strings). */
    char *const args = (char *)sp;

    strcpy(args, elf_info->argv0);

    char *const arg1 = args + strlen(elf_info->argv0) + 1;

    char *const envs = cmdline_write_arguments(arg1, cmdline);

    cmdline_write_environ(envs, cmdline);

    /* Fill in content of pointer arrays argv and envp. */
    initialize_string_array(argv, args, argc);
    initialize_string_array(envp, envs, nenv);
}

/**
 * Find an ELF section header by type
 *
 * If multiple sections of the same type are present, the first instance is
 * returned.
 *
 * @param elf_header ELF header of ELF binary
 * @param type type of symbol
 * @return pointer on section header if found, NULL otherwise
 *
 * */
static const Elf32_Shdr *elf_find_section_header_by_type(
        const Elf32_Ehdr    *elf_header,
        Elf32_Word           type) {

    for(int idx = 0; idx < elf_header->e_shnum; ++idx) {
        const Elf32_Shdr *section_header = elf_get_section_header(elf_header, idx);

        if(section_header->sh_type == type) {
            return section_header;
        }
    }

    return NULL;
}

/**
 * Find the section header for the symbol table
 *
 * @param elf_header ELF header of ELF binary
 * @return pointer on section header if found, NULL otherwise
 *
 * */
static const Elf32_Shdr *elf_find_symtab_section_header(const Elf32_Ehdr *elf_header) {
    return elf_find_section_header_by_type(elf_header, SHT_SYMTAB);
}

/**
 * Get the binary data of a section
 *
 * @param elf_header ELF header of ELF binary
 * @param section_header ELF section header
 * @return symbol name as a NUL-terminated string
 *
 * */
static const char *elf_section_data(
        const Elf32_Ehdr    *elf_header,
        const Elf32_Shdr    *section_header) {

    const char *elf_file = elf_file_bytes(elf_header);
    return &elf_file[section_header->sh_offset];
}

/**
 * Look up the name of a symbol
 *
 * @param elf_header ELF header of ELF binary
 * @param symbol_header ELF symbol header
 * @return symbol name as a NUL-terminated string
 *
 * */
const char *elf_symbol_name(
        const Elf32_Ehdr    *elf_header,
        const Elf32_Sym     *symbol_header) {

    /* Here, we can safely assume the symbol table exists because the symbol
     * header passed as argument had to be looked up there. */
    const Elf32_Shdr *string_section_header = elf_get_section_header(
            elf_header,
            elf_find_symtab_section_header(elf_header)->sh_link);

    const char *string_table = elf_section_data(elf_header, string_section_header);

    return &string_table[symbol_header->st_name];
}

/**
 * Look up a symbol in the ELF binary's symbol table by address and type
 *
 * Input is an address and a symbol type. The result contains the start address
 * and length of the symbol.
 *
 * @param elf_header ELF header of ELF binary
 * @param addr address to look up
 * @param type type of the symbol
 * @return symbol header if found, NULL otherwise
 *
 * */
const Elf32_Sym *elf_find_symbol_by_address_and_type(
        const Elf32_Ehdr    *elf_header,
        Elf32_Addr           addr,
        int                  type) {

    const Elf32_Shdr *section_header = elf_find_symtab_section_header(elf_header);

    if(section_header == NULL) {
        /* no symbol table */
        return NULL;
    }

    const char *symbols_table = elf_section_data(elf_header, section_header);
    
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
 * @param elf_header ELF header of ELF binary
 * @param addr address to look up
 * @return symbol header if found, NULL otherwise
 *
 * */
const Elf32_Sym *elf_find_function_symbol_by_address(
        const Elf32_Ehdr    *elf_header,
        Elf32_Addr           addr) {

    return elf_find_symbol_by_address_and_type(elf_header, addr, STT_FUNCTION);
}
