#ifndef _JINUE_COMMON_VM_H
#define _JINUE_COMMON_VM_H

#include <jinue-common/asm/vm.h>

#include <stdbool.h>
#include <stdint.h>


/** byte offset in page of virtual (linear) address */
#define page_offset_of(x)   ((uintptr_t)(x) & PAGE_MASK)

/** address of the page that contains a virtual (linear) address */
#define page_address_of(x)  ((uintptr_t)(x) & ~PAGE_MASK)

/** sequential page number of virtual (linear) address */
#define page_number_of(x)   ((uintptr_t)(x) >> PAGE_BITS)

/** Check whether a pointer points to kernel space */
static inline bool is_kernel_pointer(const void *addr) {
    return (uintptr_t)addr >= KLIMIT;
}

/** Check whether a pointer points to user space */
static inline bool is_user_pointer(const void *addr) {
    return (uintptr_t)addr < KLIMIT;
}

/** Check whether a pointer is in the fast path range for map/unmap operations */
static inline bool is_fast_map_pointer(const void *addr) {
    return is_kernel_pointer(addr);
}

/** Maximum size of user buffer starting at specified address */
static inline uintptr_t user_pointer_max_size(const void *addr) {
    return (uintptr_t)0 - (uintptr_t)addr;
}

/** Check that a buffer is completely in user space */
static inline bool user_buffer_check(const void *addr, uintptr_t size) {
    return is_user_pointer(addr) && size <= user_pointer_max_size(addr);
}

#endif
