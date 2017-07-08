#ifndef _JINUE_SYSCALL_H_
#define _JINUE_SYSCALL_H_

#include <jinue/pfalloc.h>
#include <jinue/types.h>
#include <stddef.h>
#include <stdint.h>

/** interrupt vector for system call software interrupt */
#define SYSCALL_IRQ    0x80

/** get best system call implementation number based on CPU features */
#define SYSCALL_FUNCT_SYSCALL_METHOD             1

/** send a character to in-kernel console driver */
#define SYSCALL_FUNCT_CONSOLE_PUTC               2

/** send a fixed-length string to in-kernel console driver */
#define SYSCALL_FUNCT_CONSOLE_PUTS               3

/** create a new thread */
#define SYSCALL_FUNCT_THREAD_CREATE              4

/** relinquish the CPU and allow the next thread to run */
#define SYSCALL_FUNCT_THREAD_YIELD               5

/** set address and size of thread local storage for current thread */
#define SYSCALL_FUNCT_SET_THREAD_LOCAL_ADDR      6

/** get address of thread local storage for current thread */
#define SYSCALL_FUNCT_GET_THREAD_LOCAL_ADDR      7

/** get free memory block list for management by process manager */
#define SYSCALL_FUNCT_GET_FREE_MEMORY            8


/** Intel's fast system call method (SYSENTER/SYSEXIT) */
#define SYSCALL_METHOD_FAST_INTEL       0

/** AMD's fast system call method (SYSCALL/SYSLEAVE) */
#define SYSCALL_METHOD_FAST_AMD         1

/** slow/safe system call method using interrupts */
#define SYSCALL_METHOD_INTR             2

/** number of bits reserved for the message buffer size and data size fields */
#define JINUE_SEND_SIZE_BITS            12

/** number of bits reserved for the number of message descriptors */
#define JINUE_SEND_N_DESC_BITS          8

/** maximum size of a message buffer and of the data inside that buffer */
#define JINUE_SEND_MAX_SIZE             (1 << (JINUE_SEND_SIZE_BITS - 1))

/** maximum number of descriptors inside a message */
#define JINUE_SEND_MAX_N_DESC           ((1 << JINUE_SEND_N_DESC_BITS) - 1)

/** mask to extract the message buffer or data size fields */
#define JINUE_SEND_SIZE_MASK            ((1 << JINUE_SEND_SIZE_BITS) - 1)

/** mask to extract the number of descriptors inside a message */
#define JINUE_SEND_N_DESC_MASK          JINUE_SEND_MAX_N_DESC

typedef struct {
    uintptr_t arg0;
    uintptr_t arg1;
    uintptr_t arg2;
    uintptr_t arg3;
} jinue_syscall_args_t;

/*  +-----------------------+------------------------+---------------+
    |      buffer_size      |       data_size        |     n_desc    |  arg3
    +-----------------------+------------------------+---------------+
    31                    20 19                     8 7              0 */

static inline uintptr_t jinue_args_pack_buffer_size(size_t buffer_size) {
    return (uintptr_t)buffer_size << (JINUE_SEND_N_DESC_BITS + JINUE_SEND_SIZE_BITS);
}

static inline uintptr_t jinue_args_pack_data_size(size_t data_size) {
    return (uintptr_t)data_size << JINUE_SEND_N_DESC_BITS;
}

static inline uintptr_t jinue_args_pack_n_desc(unsigned int n_desc) {
    return (uintptr_t)n_desc;
}

static inline char *jinue_args_get_buffer_ptr(const jinue_syscall_args_t *args) {
    return (char *)(args->arg2);
}

static inline size_t jinue_args_get_buffer_size(const jinue_syscall_args_t *args) {
    return ((size_t)(args->arg3) >> (JINUE_SEND_N_DESC_BITS + JINUE_SEND_SIZE_BITS)) & JINUE_SEND_SIZE_MASK;
}

static inline size_t jinue_args_get_data_size(const jinue_syscall_args_t *args) {
    return ((size_t)(args->arg3) >> JINUE_SEND_N_DESC_BITS) & JINUE_SEND_SIZE_MASK;
}

static inline unsigned int jinue_args_get_n_desc(const jinue_syscall_args_t *args) {
    return (unsigned int)(args->arg3) & JINUE_SEND_N_DESC_MASK;
}

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

int jinue_send(
        int              function,
        int              target,
        char            *buffer,
        size_t           buffer_size,
        size_t           data_size,
        unsigned int     n_desc,
        int             *perrno);

int jinue_get_free_memory(memory_block_t *buffer, size_t buffer_size, int *perrno);

int jinue_thread_create(void (*entry)(), void *stack, int *perrno);

int jinue_yield(void);

#endif
