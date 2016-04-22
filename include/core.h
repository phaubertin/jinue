/*
 * core.h
 *
 *  Created on: 2016-04-21
 *      Author: phil
 */

#ifndef JINUE_KERNEL_CORE_H_
#define JINUE_KERNEL_CORE_H_

#include <elf.h>

/* The following two symbols are defined by a linker script
 *
 * (see scripts/kernel-bin.lds) */
extern Elf32_Ehdr proc_elf;

extern int proc_elf_end;


void kmain(void);

#endif /* CORE_H_ */
