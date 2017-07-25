#ifndef _JINUE_IPC_H_
#define _JINUE_IPC_H_

#include <jinue-common/ipc.h>
#include <stddef.h>

typedef struct {
    uintptr_t            function;
    uintptr_t            cookie;
    size_t               buffer_size;
    size_t               data_size;
    size_t               desc_n;
} jinue_message_t;

typedef struct {
    size_t               data_size;
    size_t               desc_n;
} jinue_reply_t;

int jinue_send(
        int              function,
        int              fd,
        char            *buffer,
        size_t           buffer_size,
        size_t           data_size,
        unsigned int     n_desc,
        int             *perrno);

int jinue_receive(
        int              fd,
        char            *buffer,
        size_t           buffer_size,
        jinue_message_t *message,
        int             *perrno);

int jinue_reply(
        char            *buffer,
        size_t           buffer_size,
        size_t           data_size,
        unsigned int     n_desc,
        int             *perrno);

int jinue_create_ipc(int flags, int *perrno);

#endif
