#ifndef JINUE_HAL_CPU_DATA_H
#define JINUE_HAL_CPU_DATA_H

#include <hal/descriptors.h>
#include <hal/x86.h>
#include <types.h>


struct cpu_data_t {
    seg_descriptor_t     gdt[GDT_LENGTH];
    /* The assembly-language system call entry point for the SYSCALL instruction
     * (fast_amd_entry in syscall.asm) makes assumptions regarding the location
     * of the TSS within this structure. */
    tss_t                tss;
    struct cpu_data_t   *self;
    addr_space_t        *current_addr_space;
};

typedef struct cpu_data_t cpu_data_t;

#define CPU_DATA_ALIGNMENT      256


static inline cpu_data_t *get_cpu_local_data(void) {
    /* CPU data structure is at address zero within the per-cpu data segment */
    cpu_data_t *const zero = (cpu_data_t *)0;

    return (cpu_data_t *)get_gs_ptr( (uint32_t *)&zero->self );
}

static inline tss_t *get_tss(void) {
    return &get_cpu_local_data()->tss;
}

static inline addr_space_t *get_current_addr_space(void) {
    return get_cpu_local_data()->current_addr_space;
}

#endif
