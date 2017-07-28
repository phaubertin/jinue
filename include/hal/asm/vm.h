#ifndef JINUE_HAL_ASM_VM_H
#define JINUE_HAL_ASM_VM_H

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

#endif
