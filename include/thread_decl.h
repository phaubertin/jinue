#ifndef JINUE_KERNEL_THREAD_DECL_H
#define JINUE_KERNEL_THREAD_DECL_H

#include <jinue/ipc.h>
#include <jinue/list.h>
#include <jinue/syscall.h>
#include <hal/thread_ctx.h>
#include <message.h>
#include <object_decl.h>
#include <process.h>

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

#endif
