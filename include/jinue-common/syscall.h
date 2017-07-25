#ifndef _JINUE_COMMON_SYSCALL_H
#define _JINUE_COMMON_SYSCALL_H

#include <jinue-common/asm/syscall.h>

#include <stdint.h>

typedef struct {
    uintptr_t arg0;
    uintptr_t arg1;
    uintptr_t arg2;
    uintptr_t arg3;
} jinue_syscall_args_t;

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

#endif
