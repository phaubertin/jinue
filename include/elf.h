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


void elf_check(Elf32_Ehdr *elf);

void elf_load(elf_info_t *info, Elf32_Ehdr *elf, addr_space_t *addr_space);

void elf_setup_stack(elf_info_t *info);

#endif
