#ifndef JINUE_KERNEL_PROCESS_H
#define JINUE_KERNEL_PROCESS_H

#include <hal/vm.h>
#include <object_decl.h>

#define PROCESS_MAX_DESCRIPTORS     12

typedef struct {
    object_header_t header;
    addr_space_t    addr_space;
    object_ref_t    descriptors[PROCESS_MAX_DESCRIPTORS];
} process_t;


void process_boot_init(void);

process_t *process_create(void);

object_ref_t *process_get_descriptor(process_t *process, int fd);

int process_unused_descriptor(process_t *process);

#endif
