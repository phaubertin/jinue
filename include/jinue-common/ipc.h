#ifndef _JINUE_COMMON_IPC_H_
#define _JINUE_COMMON_IPC_H_

#include <jinue-common/asm/ipc.h>

#include <jinue-common/syscall.h>
#include <stddef.h>
#include <stdint.h>

#define JINUE_IPC_NONE      0

#define JINUE_IPC_SYSTEM    (1<<0)

#define JINUE_IPC_PROC      (1<<1)

/* TBD */
typedef int jinue_ipc_descriptor_t;


static inline uintptr_t jinue_args_pack_buffer_size(size_t buffer_size) {
    return (uintptr_t)buffer_size << JINUE_SEND_BUFFER_SIZE_OFFSET;
}

static inline uintptr_t jinue_args_pack_data_size(size_t data_size) {
    return (uintptr_t)data_size << JINUE_SEND_DATA_SIZE_OFFSET;
}

static inline uintptr_t jinue_args_pack_n_desc(unsigned int n_desc) {
    return (uintptr_t)n_desc << JINUE_SEND_N_DESC_OFFSET;
}

static inline char *jinue_args_get_buffer_ptr(const jinue_syscall_args_t *args) {
    return (char *)(args->arg2);
}

static inline size_t jinue_args_get_buffer_size(const jinue_syscall_args_t *args) {
    return ((size_t)(args->arg3) >> JINUE_SEND_BUFFER_SIZE_OFFSET) & JINUE_SEND_SIZE_MASK;
}

static inline size_t jinue_args_get_data_size(const jinue_syscall_args_t *args) {
    return ((size_t)(args->arg3) >> JINUE_SEND_DATA_SIZE_OFFSET) & JINUE_SEND_SIZE_MASK;
}

static inline unsigned int jinue_args_get_n_desc(const jinue_syscall_args_t *args) {
    return ((unsigned int)(args->arg3) >> JINUE_SEND_N_DESC_OFFSET) & JINUE_SEND_N_DESC_MASK;
}

#endif
