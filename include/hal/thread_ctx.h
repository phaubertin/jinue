#ifndef JINUE_KERNEL_HAL_THREAD_CTX_H_
#define JINUE_KERNEL_HAL_THREAD_CTX_H_

#include <jinue/types.h>
#include <hal/vm.h>
#include <stddef.h>

struct thread_context_t {
    /* The assembly language thread switching code makes the assumption that
     * saved_stack_pointer is the first member of this structure. */
    addr_t           saved_stack_pointer;
    addr_space_t    *addr_space;
    addr_t           local_storage_addr;
    size_t           local_storage_size;
};

typedef struct thread_context_t thread_context_t;

#endif
