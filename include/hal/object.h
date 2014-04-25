#ifndef _JINUE_KERNEL_OBJECT_H
#define _JINUE_KERNEL_OBJECT_H


#define OBJECT_TYPE_SYSCALL         0

#define OBJECT_TYPE_IPC             1

#define OBJECT_TYPE_PROCESS         2

#define OBJECT_TYPE_THREAD          3


#define OBJECT_FLAG_CLOSED          (1<<0)


typedef struct {
    unsigned int type;
    unsigned int flags;
    unsigned int ref_count;
} object_header_t;

#endif
