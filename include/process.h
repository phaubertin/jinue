#ifndef JINUE_KERNEL_PROCESS_H
#define JINUE_KERNEL_PROCESS_H

#include <hal/vm.h>
#include <types.h>


void process_boot_init(void);

process_t *process_create(void);

object_ref_t *process_get_descriptor(process_t *process, int fd);

int process_unused_descriptor(process_t *process);

#endif
