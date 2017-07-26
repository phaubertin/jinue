#ifndef _JINUE_SYSCALL_H
#define _JINUE_SYSCALL_H

#include <jinue-common/pfalloc.h>
#include <jinue-common/syscall.h>
#include <stddef.h>


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
