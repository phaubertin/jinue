#ifndef JINUE_HAL_TYPES_H
#define JINUE_HAL_TYPES_H

#include <jinue-common/elf.h>
#include <jinue-common/pfaddr.h>
#include <hal/asm/descriptors.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


typedef unsigned char   *addr_t;

/** incomplete struct type used for the definition of pte_t */
struct __pte_t;

/** type of a page table entry */
typedef struct __pte_t pte_t;

typedef struct {
    /* The assembly language thread switching code makes the assumption that
     * saved_stack_pointer is the first member of this structure. */
    addr_t           saved_stack_pointer;
    addr_t           local_storage_addr;
    size_t           local_storage_size;
} thread_context_t;

typedef struct {
    uint32_t     cr3;
    union {
        pfaddr_t     pd;    /* non-PAE: page directory */
        pte_t       *pdpt;  /* PAE: page directory pointer table */
    } top_level;
} addr_space_t;

typedef uint32_t e820_type_t;

typedef uint64_t e820_size_t;

typedef uint64_t e820_addr_t;

typedef struct {
    e820_addr_t addr;
    e820_size_t size;
    e820_type_t type;
} e820_t;

/* The declaration below must match the structure defined at the start of the
 * 32-bit setup code. See boot_info_struct in boot/setup32.asm. */
typedef struct {
    Elf32_Ehdr  *kernel_start;
    uint32_t     kernel_size;
    Elf32_Ehdr  *proc_start;
    uint32_t     proc_size;
    void        *image_start;
    void        *image_top;
    uint32_t     e820_entries;
    e820_t      *e820_map;
    void        *boot_heap;
    void        *boot_end;
    uint32_t     setup_signature;
} boot_info_t;

typedef uint64_t seg_descriptor_t;

typedef uint32_t seg_selector_t;

typedef struct {
    uint16_t padding;
    uint16_t limit;
    addr_t         addr;
} pseudo_descriptor_t;

typedef struct {
    /* offset 0 */
    uint16_t prev;
    /* offset 4 */
    addr_t         esp0;
    /* offset 8 */
    uint16_t ss0;
    /* offset 12 */
    addr_t         esp1;
    /* offset 16 */
    uint16_t ss1;
    /* offset 20 */
    addr_t         esp2;
    /* offset 24 */
    uint16_t ss2;
    /* offset 28 */
    uint32_t  cr3;
    /* offset 32 */
    uint32_t  eip;
    /* offset 36 */
    uint32_t  eflags;
    /* offset 40 */
    uint32_t  eax;
    /* offset 44 */
    uint32_t  ecx;
    /* offset 48 */
    uint32_t  edx;
    /* offset 52 */
    uint32_t  ebx;
    /* offset 56 */
    uint32_t  esp;
    /* offset 60 */
    uint32_t  ebp;
    /* offset 64 */
    uint32_t  esi;
    /* offset 68 */
    uint32_t  edi;
    /* offset 72 */
    uint16_t es;
    /* offset 76 */
    uint16_t cs;
    /* offset 80 */
    uint16_t ss;
    /* offset 84 */
    uint16_t ds;
    /* offset 88 */
    uint16_t fs;
    /* offset 92 */
    uint16_t gs;
    /* offset 96 */
    uint16_t ldt;
    /* offset 100 */
    uint16_t debug;
    uint16_t iomap;
} tss_t;

struct cpu_data_t {
    seg_descriptor_t     gdt[GDT_LENGTH];
    /* The assembly-language system call entry point for the SYSCALL instruction
     * (fast_amd_entry in syscall.asm) makes assumptions regarding the location
     * of the TSS within this structure. */
    tss_t                tss;
    struct cpu_data_t   *self;
    addr_space_t        *current_addr_space;
};

typedef struct cpu_data_t cpu_data_t;

#endif
