#ifndef _JINUE_KERNEL_ELF_H_
#define _JINUE_KERNEL_ELF_H_

#include <jinue/elf.h>
#include <jinue/types.h>


void elf_check(Elf32_Ehdr *);

void elf_load(Elf32_Ehdr *elf, addr_space_t *addr_space);

void elf_start(Elf32_Ehdr *elf);

#endif
