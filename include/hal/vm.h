#ifndef _JINUE_KERNEL_VM_H_
#define _JINUE_KERNEL_VM_H_

#include <stdint.h>
#include <jinue/types.h>
#include <jinue/vm.h>


/* ------ page tables ------ */

/** size of a page table */
#define PAGE_TABLE_SIZE PAGE_SIZE

/** incomplete struct type used for the definition of pte_t */
struct __pte_t;

/** type of a page table entry */
typedef struct __pte_t pte_t;


/* ------ virtual address offsets ------ */

/** bit mask for offset in page */
#define PAGE_MASK (PAGE_SIZE - 1)

/** byte offset in page of virtual (linear) address */
#define page_offset_of(x)  ((uintptr_t)(x) & PAGE_MASK)

extern addr_t page_directory_addr;

extern size_t page_table_entries;

/** page table entry offset of virtual (linear) address */
extern unsigned int (*page_table_offset_of)(addr_t);

extern unsigned int (*page_directory_offset_of)(addr_t);

extern pte_t *(*page_table_of)(addr_t);

extern pte_t *(*page_table_pte_of)(addr_t);

extern pte_t *(*get_pte)(addr_t);

extern pte_t *(*get_pde)(addr_t);

extern pte_t *(*get_pte_with_offset)(pte_t *, unsigned int);

extern void (*set_pte)(pte_t *, pfaddr_t, int);

extern void (*set_pte_flags)(pte_t *, int);

extern int (*get_pte_flags)(pte_t *);

extern pfaddr_t (*get_pte_pfaddr)(pte_t *);

extern void (*clear_pte)(pte_t *);


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

void vm_init(void);

void vm_map(addr_t vaddr, pfaddr_t paddr, int flags);

void vm_unmap(addr_t addr);

pfaddr_t vm_lookup_pfaddr(addr_t addr);

void vm_change_flags(addr_t vaddr, int flags);

void vm_map_early(addr_t vaddr, addr_t paddr, int flags, pte_t *page_directory);

pte_t *vm_create_addr_space_early(void);

#endif

