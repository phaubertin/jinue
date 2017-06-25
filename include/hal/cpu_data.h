#ifndef _JINUE_KERNEL_CPU_DATA_H_
#define _JINUE_KERNEL_CPU_DATA_H_

#include <hal/thread.h>
#include <hal/vm.h>
#include <hal/x86.h>

struct cpu_data_t {
    seg_descriptor_t     gdt[GDT_LENGTH];
    /* The assembly-language system call entry point for the SYSCALL instruction
     * (fast_amd_entry in syscall.asm) makes assumptions regarding the location
     * of the TSS within this structure. */
    tss_t                tss;
    struct cpu_data_t   *self;
    thread_context_t    *current_thread_context;
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

static inline thread_context_t *get_current_thread_context(void) {
    return get_cpu_local_data()->current_thread_context;
}

static inline addr_space_t *get_current_addr_space(void) {
    thread_context_t *thread_ctx = get_current_thread_context();

    if(thread_ctx == NULL) {
        return NULL;
    }
    else {
        return thread_ctx->addr_space;
    }
}

#endif
