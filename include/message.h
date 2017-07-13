#ifndef JINUE_KERNEL_MESSAGE_H
#define JINUE_KERNEL_MESSAGE_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uintptr_t            function;
    uintptr_t            cookie;
    size_t               buffer_size;
    size_t               data_size;
    size_t               desc_n;
    size_t               total_size;
} message_info_t;

#endif
