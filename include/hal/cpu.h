/*
 * Copyright (C) 2019 Philippe Aubertin.
 * All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the author nor the names of other contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef JINUE_HAL_CPU_H
#define JINUE_HAL_CPU_H

#include <hal/types.h>


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
