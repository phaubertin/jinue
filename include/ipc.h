#ifndef JINUE_KERNEL_IPC_H
#define JINUE_KERNEL_IPC_H

#include <types.h>


/* object header flag bits 0..7 are reserved for common flags, flag bits 8 and
 * up are usable as per object type flags */

#define IPC_FLAG_NONE           0

#define IPC_FLAG_SYSTEM         (1<<8)


void ipc_boot_init(void);

ipc_t *ipc_object_create(int flags);

ipc_t *ipc_get_proc_object(void);

void ipc_send(jinue_syscall_args_t *args);

void ipc_receive(jinue_syscall_args_t *args);

void ipc_reply(jinue_syscall_args_t *args);

#endif
