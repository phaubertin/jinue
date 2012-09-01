#ifndef _JINUE_ELF_H_
#define _JINUE_ELF_H_

/** 0x7f 'E' 'L' 'F' */
#define ELF_MAGIC 0x464c457f


/** File identification - byte 0 (0x7f) */
#define EI_MAG0 0

/** File identification - byte 1 ('E') */
#define EI_MAG1 1

/** File identification - byte 2 ('L') */
#define EI_MAG2 2

/** File identification - byte 3 ('F') */
#define EI_MAG3 3

/** File class */
#define EI_CLASS 4

/** Data encoding */
#define EI_DATA 5

/** File version  */
#define EI_VERSION 6

/** Start of padding bytes */
#define EI_PAD 7

/** size of e_ident[] */
#define EI_NIDENT  16


/** No machine */
#define EM_NONE 0

/** AT&T WE 32100 */
#define EM_M32 1

/** SPARC */
#define EM_SPARC 2

/** Intel 80386 */
#define EM_386 3

/** Motorola 68000 */
#define EM_68K 4

/** Motorola 88000 */
#define EM_88K 5

/** Intel 80860 */
#define EM_860 7

/** MIPS RS3000 */
#define EM_MIPS 8

/** Enhanced instruction set SPARC */
#define EM_SPARC32PLUS 18


/** No file type */
#define ET_NONE 0

/** Relocatable file */
#define ET_REL 1

/** Executable file */
#define ET_EXEC 2

/** Shared object file */
#define ET_DYN 3

/** Core file */
#define ET_CORE 4


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


#define PT_NULL 0

#define PT_LOAD 1

#define PT_DYNAMIC 2

#define PT_INTERP 3

#define PT_NOTE 4

#define PT_SHLIB 5

#define PT_PHDR  6


#define PF_R (1 << 2)

#define PF_W (1 << 1)

#define PF_X (1 << 0)

typedef void *Elf32_Addr;

typedef unsigned short Elf32_Half;

typedef unsigned int Elf32_Off;

typedef signed int Elf32_Sword;

typedef unsigned int Elf32_Word;

typedef int (*elf_entry_t)(void);

typedef struct {
        unsigned char e_ident[EI_NIDENT];
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


#endif
