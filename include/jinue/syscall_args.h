#ifndef _JINUE_SYSCALL_ARGS_H
#define _JINUE_SYSCALL_ARGS_H

#include <stdint.h>

typedef struct {
    uintptr_t arg0;
    uintptr_t arg1;
    uintptr_t arg2;
    uintptr_t arg3;
} jinue_syscall_args_t;

#endif
