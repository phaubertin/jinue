#ifndef _JINUE_KERNEL_VM_H_
#define _JINUE_KERNEL_VM_H_

#include <stdint.h>
#include <stdbool.h>
#include <jinue/page_tables.h>
#include <jinue/types.h>
#include <jinue/vm.h>


/* ------ flags for page attributes ------ */

/** page is present in memory */
#define VM_FLAG_PRESENT       (1<< 0)

/** page is read only */
#define VM_FLAG_READ_ONLY     0

/** page is read/write accessible */
#define VM_FLAG_READ_WRITE    (1<< 1)

/** kernel mode page (default) */
#define VM_FLAG_KERNEL        0

/** user mode page */
#define VM_FLAG_USER          (1<< 2)

/** write-through cache policy for page */
#define VM_FLAG_WRITE_THROUGH (1<< 3)

/** uncached page */
#define VM_FLAG_CACHE_DISABLE (1<< 4)

/** page was accessed (read) */
#define VM_FLAG_ACCESSED      (1<< 5)

/** page was written to */
#define VM_FLAG_DIRTY         (1<< 6)

/** page directory entry describes a 4M page */
#define VM_FLAG_BIG_PAGE      (1<< 7)

/** page is global (mapped in every address space) */
#define VM_FLAG_GLOBAL        (1<< 8)

/** set of flags for a page table (or page directory) */
#define VM_FLAGS_PAGE_TABLE (VM_FLAG_KERNEL | VM_FLAG_READ_WRITE)


typedef struct {
    uint32_t     cr3;
    pte_t       *top_level;
} addr_space_t;


extern bool vm_use_pae;

extern addr_t page_directory_addr;

extern size_t page_table_entries;

/** page table entry offset of virtual (linear) address */
extern unsigned int (*page_table_offset_of)(addr_t);

extern unsigned int (*global_page_table_offset_of)(addr_t);

extern unsigned int (*page_directory_offset_of)(addr_t);

extern pte_t *(*page_table_of)(addr_t);

extern pte_t *(*get_pte)(addr_t);

extern pte_t *(*get_pde)(addr_t);

extern void (*alloc_page_table)(addr_t);


void vm_init(void);

void vm_map(addr_t vaddr, pfaddr_t paddr, int flags);

void vm_unmap(addr_t addr);

pfaddr_t vm_lookup_pfaddr(addr_t addr);

void vm_change_flags(addr_t vaddr, int flags);

void vm_map_early(addr_t vaddr, addr_t paddr, int flags, pte_t *page_directory);

pte_t *vm_create_initial_addr_space(void);

#endif

