#ifndef _JINUE_KERNEL_CPU_DATA_H_
#define _JINUE_KERNEL_CPU_DATA_H_

#include <hal/vm.h>
#include <hal/x86.h>


struct cpu_data_t {
    seg_descriptor_t     gdt[GDT_LENGTH];
    tss_t                tss;
    struct cpu_data_t   *self;
    addr_space_t        *current_addr_space;
} ;

typedef struct cpu_data_t cpu_data_t;

#define CPU_DATA_ALIGNMENT      256

#define get_cpu_local_data() ( (cpu_data_t *)get_gs_ptr( (uint32_t *)&( ((cpu_data_t *)0)->self ) ) )

#define get_current_addr_space() ( get_cpu_local_data().current_addr_space )

#endif
