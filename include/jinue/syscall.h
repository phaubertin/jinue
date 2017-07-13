#ifndef _JINUE_SYSCALL_H
#define _JINUE_SYSCALL_H

#include <jinue/asm/syscall.h>

#include <jinue/pfalloc.h>
#include <jinue/syscall_args.h>
#include <jinue/types.h>
#include <stddef.h>
#include <stdint.h>


static inline uintptr_t jinue_get_return_uintptr(const jinue_syscall_args_t *args) {
    return args->arg0;
}

static inline int jinue_get_return(const jinue_syscall_args_t *args) {
    return (int)jinue_get_return_uintptr(args);
}

static inline void *jinue_get_return_ptr(const jinue_syscall_args_t *args) {
    return (void *)jinue_get_return_uintptr(args);
}

static inline int jinue_get_error(const jinue_syscall_args_t *args) {
    return (int)args->arg1;
}


void jinue_call_raw(jinue_syscall_args_t *args);

int jinue_call(jinue_syscall_args_t *args, int *perrno);

void jinue_get_syscall_implementation(void);

const char *jinue_get_syscall_implementation_name(void);

void jinue_set_thread_local_storage(void *addr, size_t size);

void *jinue_get_thread_local_storage(void);

int jinue_get_free_memory(memory_block_t *buffer, size_t buffer_size, int *perrno);

int jinue_thread_create(void (*entry)(), void *stack, int *perrno);

int jinue_yield(void);

void jinue_thread_exit(void);


#endif
