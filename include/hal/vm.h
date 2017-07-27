#ifndef JINUE_HAL_VM_H
#define JINUE_HAL_VM_H

#include <jinue-common/vm.h>
#include <hal/types.h>


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


extern pte_t *global_page_tables;

extern addr_space_t initial_addr_space;


void vm_boot_init(void);

void vm_map(addr_space_t *addr_space, addr_t vaddr, pfaddr_t paddr, int flags);

void vm_unmap(addr_space_t *addr_space, addr_t addr);

pfaddr_t vm_lookup_pfaddr(addr_space_t *addr_space, addr_t addr);

void vm_change_flags(addr_space_t *addr_space, addr_t addr, int flags);

void vm_map_early(addr_t vaddr, addr_t paddr, int flags);

addr_space_t *vm_create_addr_space(addr_space_t *addr_space);

addr_space_t *vm_create_initial_addr_space(void);

void vm_destroy_addr_space(addr_space_t *addr_space);

void vm_switch_addr_space(addr_space_t *addr_space);


#define vm_map_current(vaddr, paddr, flags) \
    vm_map(current_addr_space, vaddr, paddr, flags)

#define vm_unmap_current(addr) \
    vm_unmap(get_current_addr_space, addr)

#define vm_map_global(vaddr, paddr, flags) \
    vm_map(NULL, vaddr, paddr, flags)

#define vm_unmap_global(addr) \
    vm_unmap(NULL, addr)

#endif

