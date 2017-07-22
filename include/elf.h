#ifndef JINUE_KERNEL_ELF_H
#define JINUE_KERNEL_ELF_H

#include <jinue/elf.h>
#include <jinue/types.h>
#include <hal/vm.h>


typedef struct {
    addr_t           entry;
    addr_t           stack_addr;
    addr_t           at_phdr;
    int              at_phent;
    int              at_phnum;
    addr_space_t    *addr_space;
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


void elf_check(Elf32_Ehdr *elf);

void elf_load(elf_info_t *info, Elf32_Ehdr *elf, addr_space_t *addr_space);

void elf_setup_stack(elf_info_t *info);

int elf_lookup_symbol(
        const Elf32_Ehdr    *elf_header,
        Elf32_Addr           addr,
        int                  type,
        elf_symbol_t        *result);

#endif
