#ifndef JINUE_HAL_TYPES_H
#define JINUE_HAL_TYPES_H

#include <jinue-common/elf.h>
#include <jinue-common/pfaddr.h>
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

#endif
