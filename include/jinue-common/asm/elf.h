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

#ifndef JINUE_COMMON_ASM_ELF_H
#define JINUE_COMMON_ASM_ELF_H

/** Index of file identification - byte 0 */
#define EI_MAG0          0

/** Index of file identification - byte 1 */
#define EI_MAG1          1

/** Index of file identification - byte 2 */
#define EI_MAG2          2

/** Index of file identification - byte 3 */
#define EI_MAG3          3

/** File class */
#define EI_CLASS         4

/** Data encoding */
#define EI_DATA          5

/** File version  */
#define EI_VERSION       6

/** Start of padding bytes */
#define EI_PAD           7

/** size of e_ident[] */
#define EI_NIDENT       16


/** File identification - byte 0 (0x7f) */
#define ELF_MAGIC0       0x7f

/** File identification - byte 1 ('E') */
#define ELF_MAGIC1       'E'

/** File identification - byte 2 ('L') */
#define ELF_MAGIC2       'L'

/** File identification - byte 3 ('F') */
#define ELF_MAGIC3       'F'


/** No machine */
#define EM_NONE           0

/** SPARC */
#define EM_SPARC          2

/** Intel 80386 */
#define EM_386            3

/** MIPS RS3000 */
#define EM_MIPS           8

/** Enhanced instruction set SPARC */
#define EM_SPARC32PLUS   18

/** 32-bit ARM */
#define EM_ARM           40

/** AMD64/X86-64  */
#define EM_X86_64        62

/** OpenRISC 32-bit embedded processor */
#define EM_OPENRISC      92

/** Altera Nios 2 32-bit soft processor */
#define EM_ALTERA_NIOS2 113

/** 64-bit AARCH64 ARM */
#define EM_AARCH64      183

/** Xilinx MicroBlaze 32-bit soft processor */
#define EM_MICROBLAZE   189


/** No file type */
#define ET_NONE         0

/** Relocatable file */
#define ET_REL          1

/** Executable file */
#define ET_EXEC         2

/** Shared object file */
#define ET_DYN          3

/** Core file */
#define ET_CORE         4


/** Invalid class */
#define ELFCLASSNONE    0

/** 32-bit objects */
#define ELFCLASS32      1

/** 64-bit objects */
#define ELFCLASS64      2


/** Invalid data encoding */
#define ELFDATANONE     0

/** Little-endian */
#define ELFDATA2LSB     1

/** Big-endian  */
#define ELFDATA2MSB     2


/** Unused entry */
#define PT_NULL         0

/** Loadable segment */
#define PT_LOAD         1

/** Dynamic linking information */
#define PT_DYNAMIC      2

/** Path to program interpreter */
#define PT_INTERP       3

/** Location and size of notes */
#define PT_NOTE         4

/** Unspecified semantics */
#define PT_SHLIB        5

/** Program header table */
#define PT_PHDR         6


/** Inactive section */
#define SHT_NULL        0

/** Program data */
#define SHT_PROGBITS    1

/** Symbol table */
#define SHT_SYMTAB      2

/** String table */
#define SHT_STRTAB      3

/** Relocations with addends */
#define SHT_RELA        4

/** Symbol hash table */
#define SHT_HASH        5

/** Information for dynamic linking */
#define SHT_DYNAMIC     6

/** Notes section */
#define SHT_NOTE        7

/** Section without data (.bss) */
#define SHT_NOBITS      8

/** Relocations without addends */
#define SHT_REL         9

/** Reserved, unspecified semantic, not ABI compliant */
#define SHT_SHLIB       10

/** Dynamic symbols table */
#define SHT_DYNSYM      11


/** Local binding */
#define STB_LOCAL       0

/** Global binding */
#define STB_GLOBAL      1

/** Weak binding */
#define STB_WEAK        2


/** Unspecified type */
#define STT_NOTYPE      0

/** Data object */
#define STT_OBJECT      1

/** Function or other executable code */
#define STT_FUNCTION    2

/** Section symbol */
#define STT_SECTION     3

/** Source file */
#define STT_FILE        4


#define ELF32_ST_BIND(i)    ((i) >> 4)

#define ELF32_ST_TYPE(i)    ((i) & 0xf)


/** Undefined symbol index */
#define STN_UNDEF       0


#define PF_R            (1 << 2)

#define PF_W            (1 << 1)

#define PF_X            (1 << 0)


/** Last entry  */
#define AT_NULL          0

/** Ignore entry */
#define AT_IGNORE        1

/** Program file descriptor */
#define AT_EXECFD        2

/** Program headers address */
#define AT_PHDR          3

/** Size of program header entry */
#define AT_PHENT         4

/** Number of program header entries */
#define AT_PHNUM         5

/** Page size */
#define AT_PAGESZ        6

/** Base address */
#define AT_BASE          7

/** Flags */
#define AT_FLAGS         8

/** Program entry point */
#define AT_ENTRY         9

/** Data cache block size */
#define AT_DCACHEBSIZE  10

/** Instruction cache block size */
#define AT_ICACHEBSIZE  11

/** Unified cache block size */
#define AT_UCACHEBSIZE  12

/** Stack base address for main thread */
#define AT_STACKBASE    13

/** Machine-dependent processor feature flags */
#define AT_HWCAP        16

/** More machine-dependent processor feature flags */
#define AT_HWCAP2       26

/** Address of vDSO */
#define AT_SYSINFO_EHDR 33


/** Offset of e_entry field in Elf32_Ehdr */
#define ELF_E_ENTRY     24

/** Offset of e_phoff field in Elf32_Ehdr */
#define ELF_E_PHOFF     28

/* Offset of e_phnum in Elf32_Ehdr */
#define ELF_E_PHNUM     44

/* Offset of p_type in Elf32_Phdr */
#define ELF_P_TYPE      0

/* Offset of p_offset in Elf32_Phdr */
#define ELF_P_OFFSET    4

/* Offset of p_vaddr in Elf32_Phdr */
#define ELF_P_VADDR     8

/* Offset of p_paddr in Elf32_Phdr */
#define ELF_P_PADDR     12

/* Offset of p_filesz in Elf32_Phdr */
#define ELF_P_FILESZ    16

/* Offset of p_memsz in Elf32_Phdr */
#define ELF_P_MEMSZ     20

/* Offset of p_flags in Elf32_Phdr */
#define ELF_P_FLAGS     24

/* Offset of p_align in Elf32_Phdr */
#define ELF_P_ALIGN     28

/* Size of Elf32_Phdr */
#define ELF_PHDR_SIZE   32

#endif
