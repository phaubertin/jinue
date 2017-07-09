#ifndef JINUE_HAL_CPU_H
#define JINUE_HAL_CPU_H

#include <hal/cpu_data.h>
#include <hal/descriptors.h>
#include <stdbool.h>

#define MSR_IA32_SYSENTER_CS        0x174

#define MSR_IA32_SYSENTER_ESP       0x175

#define MSR_IA32_SYSENTER_EIP       0x176

#define MSR_EFER                    0xC0000080

#define MSR_STAR                    0xC0000081


#define MSR_FLAG_STAR_SCE           (1<<0)


#define CPU_FEATURE_CPUID           (1<<0)

#define CPU_FEATURE_SYSENTER        (1<<1)

#define CPU_FEATURE_SYSCALL         (1<<2)

#define CPU_FEATURE_LOCAL_APIC      (1<<3)

#define CPU_FEATURE_PAE             (1<<4)


#define CPU_EFLAGS_ID               (1<<21)


#define CPUID_FEATURE_FPU           (1<<0)

#define CPUID_FEATURE_PAE           (1<<6)

#define CPUID_FEATURE_APIC          (1<<9)

#define CPUID_FEATURE_SEP           (1<<11)

#define CPUID_FEATURE_CLFLUSH       (1<<19)

#define CPUID_FEATURE_HTT           (1<<28)


#define CPUID_EXT_FEATURE_SYSCALL   (1<<11)


#define CPU_VENDOR_GENERIC          0

#define CPU_VENDOR_AMD              1

#define CPU_VENDOR_INTEL            2


#define CPU_VENDOR_AMD_DW0          0x68747541    /* Auth */
#define CPU_VENDOR_AMD_DW1          0x69746e65    /* enti */
#define CPU_VENDOR_AMD_DW2          0x444d4163    /* cAMD */

#define CPU_VENDOR_INTEL_DW0        0x756e6547    /* Genu */
#define CPU_VENDOR_INTEL_DW1        0x49656e69    /* ineI */
#define CPU_VENDOR_INTEL_DW2        0x6c65746e    /* ntel */

typedef struct {
    unsigned int     dcache_alignment;
    uint32_t         features;
    int              vendor;
    int              family;
    int              model;
    int              stepping;
} cpu_info_t;

extern cpu_info_t cpu_info;

static inline bool cpu_has_feature(uint32_t mask) {
    return (cpu_info.features & mask) == mask;
}

void cpu_init_data(cpu_data_t *data, addr_t kernel_stack);

void cpu_detect_features(void);


#endif
