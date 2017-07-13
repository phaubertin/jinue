#include <jinue/errno.h>
#include <jinue/ipc.h>
#include <jinue/syscall.h>

static inline void set_errno(int *perrno, int errval) {
    if(perrno != NULL) {
        *perrno = errval;
    }
}

int jinue_send(
        int              function,
        int              fd,
        char            *buffer,
        size_t           buffer_size,
        size_t           data_size,
        unsigned int     n_desc,
        int             *perrno) {

    jinue_syscall_args_t args;

    /* The library has to perform this check and set the appropriate error
     * because the kernel cannot check this once the values have been packed. */
    if(data_size > JINUE_SEND_MAX_SIZE || n_desc > JINUE_SEND_MAX_N_DESC) {
        set_errno(perrno, JINUE_EINVAL);

        return -1;
    }

    /* Silently crop the buffer size if it is greater than the maximum allowed. */
    if(buffer_size > JINUE_SEND_MAX_SIZE) {
        buffer_size = JINUE_SEND_MAX_SIZE;
    }

    args.arg0 = (uintptr_t)function;
    args.arg1 = (uintptr_t)fd;
    args.arg2 = (uintptr_t)buffer;
    args.arg3 =     jinue_args_pack_buffer_size(buffer_size)
                  | jinue_args_pack_data_size(data_size)
                  | jinue_args_pack_n_desc(n_desc);

    return jinue_call(&args, perrno);

    /** TODO handle reply data size/n_desc */
}

int jinue_receive(
        int              fd,
        char            *buffer,
        size_t           buffer_size,
        jinue_message_t *message,
        int             *perrno) {

    jinue_syscall_args_t args;

    /* Silently crop the buffer size if it is greater than the maximum allowed. */
    if(buffer_size > JINUE_SEND_MAX_SIZE) {
        buffer_size = JINUE_SEND_MAX_SIZE;
    }

    args.arg0 = SYSCALL_FUNCT_RECEIVE;
    args.arg1 = (uintptr_t)fd;
    args.arg2 = (uintptr_t)buffer;
    args.arg3 = jinue_args_pack_buffer_size(buffer_size);

    jinue_call_raw(&args);

    if(args.arg0 == (uintptr_t)-1) {
        set_errno(perrno, (int)args.arg1);
        return -1;
    }

    message->function       = args.arg0;
    message->cookie         = args.arg1;
    message->buffer_size    = jinue_args_get_buffer_size(&args);
    message->data_size      = jinue_args_get_data_size(&args);
    message->desc_n         = jinue_args_get_n_desc(&args);

    return 0;
}

int jinue_reply(
        char            *buffer,
        size_t           buffer_size,
        size_t           data_size,
        unsigned int     n_desc,
        int             *perrno) {

    jinue_syscall_args_t args;

    /* The library has to perform this check and set the appropriate error
     * because the kernel cannot check this once the values have been packed. */
    if(data_size > JINUE_SEND_MAX_SIZE || n_desc > JINUE_SEND_MAX_N_DESC) {
        set_errno(perrno, JINUE_EINVAL);

        return -1;
    }

    /* Silently crop the buffer size if it is greater than the maximum allowed. */
    if(buffer_size > JINUE_SEND_MAX_SIZE) {
        buffer_size = JINUE_SEND_MAX_SIZE;
    }

    args.arg0 = SYSCALL_FUNCT_REPLY;
    args.arg1 = 0;
    args.arg2 = (uintptr_t)buffer;
    args.arg3 =     jinue_args_pack_buffer_size(buffer_size)
                  | jinue_args_pack_data_size(data_size)
                  | jinue_args_pack_n_desc(n_desc);

    return jinue_call(&args, perrno);
}

int jinue_create_ipc(int flags, int *perrno) {
    jinue_syscall_args_t args;

    args.arg0 = SYSCALL_FUNCT_CREATE_IPC;
    args.arg1 = (uintptr_t)flags;
    args.arg2 = 0;
    args.arg3 = 0;

    return jinue_call(&args, perrno);
}
