#ifndef _JINUE_KERNEL_ELF_H_
#define _JINUE_KERNEL_ELF_H_

/** 0x7f 'E' 'L' 'F' */
#define ELF_MAGIC 0x464c457f


/** File identification - byte 0 (0x7f) */
#define ELF_EI_MAG0 0

/** File identification - byte 1 ('E') */
#define ELF_EI_MAG1 1

/** File identification - byte 2 ('L') */
#define ELF_EI_MAG2 2

/** File identification - byte 3 ('F') */
#define ELF_EI_MAG3 3

/** File class */
#define ELF_EI_CLASS 4

/** Data encoding */
#define ELF_EI_DATA 5

/** File version  */
#define ELF_EI_VERSION 6

/** Start of padding bytes */
#define ELF_EI_PAD 7

/** size of e_ident[] */
#define ELF_EI_NIDENT  16


/** No machine */
#define ELF_EM_NONE 0

/** AT&T WE 32100 */
#define ELF_EM_M32 1

/** SPARC */
#define ELF_EM_SPARC 2

/** Intel 80386 */
#define ELF_EM_386 3

/** Motorola 68000 */
#define ELF_EM_68K 4

/** Motorola 88000 */
#define ELF_EM_88K 5

/** Intel 80860 */
#define ELF_EM_860 7

/** MIPS RS3000 */
#define ELF_EM_MIPS 8

/** Enhanced instruction set SPARC */
#define ELF_EM_SPARC32PLUS 18


/** No file type */
#define ELF_ET_NONE 0

/** Relocatable file */
#define ELF_ET_REL 1

/** Executable file */
#define ELF_ET_EXEC 2

/** Shared object file */
#define ELF_ET_DYN 3

/** Core file */
#define ELF_ET_CORE 4


/** Invalid class */
#define ELFCLASSNONE 0

/** 32-bit objects */
#define ELFCLASS32 1

/** 64-bit objects */
#define ELFCLASS64 2


/** Invalid data encoding */
#define ELFDATANONE 0

/** little-endian */
#define ELFDATA2LSB 1

/** big-endian  */
#define ELFDATA2MSB 2


#define ELF_PT_NULL 0

#define ELF_PT_LOAD 1

#define ELF_PT_DYNAMIC 2

#define ELF_PT_INTERP 3

#define ELF_PT_NOTE 4

#define ELF_PT_SHLIB 5

#define ELF_PT_PHDR  6


#define PF_R (1 << 2)

#define PF_W (1 << 1)

#define PF_X (1 << 0)

typedef void *Elf32_Addr;

typedef unsigned short Elf32_Half;

typedef unsigned int Elf32_Off;

typedef signed int Elf32_Sword;

typedef unsigned int Elf32_Word;

typedef struct {
        unsigned char e_ident[ELF_EI_NIDENT];
        Elf32_Half    e_type;
        Elf32_Half    e_machine;
        Elf32_Word    e_version;
        Elf32_Addr    e_entry;
        Elf32_Off     e_phoff;
        Elf32_Off     e_shoff;
        Elf32_Word    e_flags;
        Elf32_Half    e_ehsize;
        Elf32_Half    e_phentsize;
        Elf32_Half    e_phnum;
        Elf32_Half    e_shentsize;
        Elf32_Half    e_shnum;
        Elf32_Half    e_shstrndx;
} elf_header_t;

typedef struct {
        Elf32_Word p_type;
        Elf32_Off  p_offset;
        Elf32_Addr p_vaddr;
        Elf32_Addr p_paddr;
        Elf32_Word p_filesz;
        Elf32_Word p_memsz;
        Elf32_Word p_flags;
        Elf32_Word p_align;
} elf_prog_header_t;


const char *elf_machine(Elf32_Half e_machine);

const char *elf_type(Elf32_Half e_type);

const char *elf_ptype(Elf32_Word p_type);

const char *elf_flags(Elf32_Word p_flags);


#endif
