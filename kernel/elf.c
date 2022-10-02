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

#include <hal/vm.h>
#include <assert.h>
#include <boot.h>
#include <cmdline.h>
#include <elf.h>
#include <page_alloc.h>
#include <panic.h>
#include <printk.h>
#include <vmalloc.h>
#include <string.h>
#include <util.h>

static bool check_failed(const char *message) {
    printk("Invalid ELF binary: %s", message);
    return false;
}

/**
 * Check the validity of an ELF binary
 *
 * @param elf ELF header, which is at the very beginning of the ELF binary
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
        elf_info_t      *info,
        Elf32_Ehdr      *elf,
        const char      *argv0,
        const char      *cmdline,
        addr_space_t    *addr_space,
        boot_alloc_t    *boot_alloc) {
    
    /* get the program header table */
    Elf32_Phdr *phdr = (Elf32_Phdr *)((char *)elf + elf->e_phoff);
    
    info->at_phdr       = (addr_t)phdr;
    info->at_phnum      = elf->e_phnum;
    info->at_phent      = elf->e_phentsize;
    info->addr_space    = addr_space;
    info->entry         = (void *)elf->e_entry;
    info->argv0         = argv0;

    for(unsigned int idx = 0; idx < elf->e_phnum; ++idx) {
        if(phdr[idx].p_type != PT_LOAD) {
            continue;
        }
        
        /* check that the segment is not in the region reserved for kernel use */
        if(! check_userspace_buffer((void *)phdr[idx].p_vaddr, phdr[idx].p_memsz)) {
            panic("process manager memory layout -- address of segment too low");
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
    
    elf_allocate_stack(info, boot_alloc);

    elf_initialize_stack(info, cmdline);
    
    printk("ELF binary loaded.\n");
}

/**
 * Allocate the stack for ELF binary
 *
 * @param info ELF information structure (output)
 * @param boot_alloc boot-time page allocator
 *
 * */
void elf_allocate_stack(elf_info_t *info, boot_alloc_t *boot_alloc) {
    /** TODO: check for overlap of stack with loaded segments */

    for(addr_t vpage = (addr_t)STACK_START; vpage < (addr_t)STACK_BASE; vpage += PAGE_SIZE) {
        void *page = page_alloc();

        /* This newly allocated page may have data left from a previous boot
         * which may contain sensitive information. Let's clear it. */
        clear_page(page);

        checked_map_userspace(
                info->addr_space,
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
void elf_initialize_stack(elf_info_t *info, const char *cmdline) {
    uintptr_t *sp       = (uintptr_t *)(STACK_BASE - RESERVED_STACK_SIZE);
    info->stack_addr    = sp;

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
    sp = (uintptr_t *)(auxvp + 7);

    auxvp[0].a_type     = AT_PHDR;
    auxvp[0].a_un.a_val = (uint32_t)info->at_phdr;

    auxvp[1].a_type     = AT_PHENT;
    auxvp[1].a_un.a_val = (uint32_t)info->at_phent;

    auxvp[2].a_type     = AT_PHNUM;
    auxvp[2].a_un.a_val = (uint32_t)info->at_phnum;

    auxvp[3].a_type     = AT_PAGESZ;
    auxvp[3].a_un.a_val = PAGE_SIZE;

    auxvp[4].a_type     = AT_ENTRY;
    auxvp[4].a_un.a_val = (uint32_t)info->entry;

    auxvp[5].a_type     = AT_STACKBASE;
    auxvp[5].a_un.a_val = STACK_BASE;

    auxvp[6].a_type     = AT_NULL;
    auxvp[6].a_un.a_val = 0;

    /* Write arguments and environment variables (i.e. the actual strings). */
    char *const args = (char *)sp;

    strcpy(args, info->argv0);

    char *const arg1 = args + strlen(info->argv0) + 1;

    char *const envs = cmdline_write_arguments(arg1, cmdline);

    cmdline_write_environ(envs, cmdline);

    /* Fill in content of pointer arrays argv and envp. */
    initialize_string_array(argv, args, argc);
    initialize_string_array(envp, envs, nenv);
}

/**
 * Look up a symbol in the ELF binary's symbol table by address
 *
 * Input is an address and a symbol type. The result contains the start address
 * and length of the symbol.
 *
 * @param elf_header ELF header of ELF binary
 * @param addr address to look up
 * @param type type of the symbol
 * @param result result variable, written only if found
 * @return 0 if symbol is found, -1 otherwise
 *
 * */
int elf_lookup_symbol(
        const Elf32_Ehdr    *elf_header,
        Elf32_Addr           addr,
        int                  type,
        elf_symbol_t        *result) {

    int     idx;
    size_t  symbol_entry_size;
    size_t  symbol_table_size;
    
    const char *elf_file        = elf_file_bytes(elf_header);
    const char *symbols_table   = NULL;
    const char *string_table    = NULL;
    
    for(idx = 0; idx < elf_header->e_shnum; ++idx) {
        const Elf32_Shdr *section_header = elf_get_section_header(elf_header, idx);
        
        if(section_header->sh_type == SHT_SYMTAB) {
            symbols_table       = &elf_file[section_header->sh_offset];
            symbol_entry_size   = section_header->sh_entsize;
            symbol_table_size   = section_header->sh_size;
            
            const Elf32_Shdr *string_section_header = elf_get_section_header(
                    elf_header,
                    section_header->sh_link);
                    
            string_table = &elf_file[string_section_header->sh_offset];

            break;
        }
    }
    
    if(symbols_table == NULL) {
        /* no symbol table */
        return -1;
    }
    
    const char *symbol = symbols_table;
    
    while(symbol < symbols_table + symbol_table_size) {
        const Elf32_Sym *symbol_header = (const Elf32_Sym *)symbol;
        
        if(ELF32_ST_TYPE(symbol_header->st_info) == type) {
            Elf32_Addr lookup_addr  = (Elf32_Addr)addr;
            Elf32_Addr start        = symbol_header->st_value;
            Elf32_Addr end          = start + symbol_header->st_size;
            
            if(lookup_addr >= start && lookup_addr < end) {
                result->addr = symbol_header->st_value;
                result->name = &string_table[symbol_header->st_name];
                
                return 0;
            }
        }
        
        symbol += symbol_entry_size;
    }
    
    /* not found */
    return -1;    
}
