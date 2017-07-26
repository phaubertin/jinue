#ifndef JINUE_KERNEL_TYPES_H
#define JINUE_KERNEL_TYPES_H

#include <jinue-common/ipc.h>
#include <jinue-common/list.h>
#include <jinue-common/pfaddr.h>
#include <jinue-common/syscall.h>
#include <jinue-common/types.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>


typedef unsigned char   *addr_t;

/** incomplete struct type used for the definition of pte_t */
struct __pte_t;

/** type of a page table entry */
typedef struct __pte_t pte_t;

typedef struct {
    int type;
    int ref_count;
    int flags;
} object_header_t;

typedef struct {
    object_header_t *object;
    uintptr_t        flags;
    uintptr_t        cookie;
} object_ref_t;

typedef struct {
    /* The assembly language thread switching code makes the assumption that
     * saved_stack_pointer is the first member of this structure. */
    addr_t           saved_stack_pointer;
    addr_t           local_storage_addr;
    size_t           local_storage_size;
} thread_context_t;

typedef struct {
    uint32_t     cr3;
    union {
        pfaddr_t     pd;    /* non-PAE: page directory */
        pte_t       *pdpt;  /* PAE: page directory pointer table */
    } top_level;
} addr_space_t;

#define PROCESS_MAX_DESCRIPTORS     12

typedef struct {
    object_header_t header;
    addr_space_t    addr_space;
    object_ref_t    descriptors[PROCESS_MAX_DESCRIPTORS];
} process_t;

typedef struct {
    uintptr_t            function;
    uintptr_t            cookie;
    size_t               buffer_size;
    size_t               data_size;
    size_t               desc_n;
    size_t               total_size;
} message_info_t;

struct thread_t {
    object_header_t          header;
    thread_context_t         thread_ctx;
    jinue_node_t             thread_list;
    process_t               *process;
    struct thread_t         *sender;
    jinue_syscall_args_t    *message_args;
    message_info_t           message_info;
    char                     message_buffer[JINUE_SEND_MAX_SIZE];
};

typedef struct thread_t thread_t;

typedef struct {
    object_header_t header;
    jinue_list_t    send_list;
    jinue_list_t    recv_list;
} ipc_t;

#endif
