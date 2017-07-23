#ifndef JINUE_HAL_THREAD_CTX_H
#define JINUE_HAL_THREAD_CTX_H

#include <hal/vm.h>
#include <stddef.h>
#include <types.h>

typedef struct {
    /* The assembly language thread switching code makes the assumption that
     * saved_stack_pointer is the first member of this structure. */
    addr_t           saved_stack_pointer;
    addr_t           local_storage_addr;
    size_t           local_storage_size;
} thread_context_t;

#endif
