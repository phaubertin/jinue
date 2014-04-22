#ifndef _JINUE_KERNEL_ELF_H_
#define _JINUE_KERNEL_ELF_H_

#include <jinue/elf.h>
#include <jinue/types.h>


extern elf_header_t proc_elf;

extern linker_defined_t proc_elf_end;


void elf_check_process_manager(void);

void elf_load_process_manager(addr_space_t *addr_space);

void elf_start_process_manager(void);

#endif
