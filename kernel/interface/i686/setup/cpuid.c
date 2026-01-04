/*
 * Copyright (C) 2026 Philippe Aubertin.
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

#include <kernel/infrastructure/i686/asm/cpuid.h>
#include <kernel/infrastructure/i686/asm/cpuinfo.h>
#include <kernel/infrastructure/i686/isa/cpuid.h>
#include <kernel/interface/i686/asm/bootinfo.h>
#include <kernel/interface/i686/setup/cpuid.h>
#include <stdbool.h>

typedef struct {
    int         id;
    uint32_t    signature_ebx;
    uint32_t    signature_ecx;
    uint32_t    signature_edx;
} cpuid_signature_t;

#define SIGNATURE_ANY (-1)

/**
 * Map a CPUID signature to an ID for the kernel's internal use
 * 
 * For use with:
 *  - Vendor signature in CPUID leaf 0x00000000
 *  - Hypervisor signature in CPUID leaf 0x40000000
 * 
 * @param regs relevant CPUID leaf
 * @param mapping mapping entries
 */
static int map_signature(const x86_cpuid_regs_t *regs, const cpuid_signature_t mapping[]) {
    for(int idx = 0;; ++idx) {
        const cpuid_signature_t *entry = &mapping[idx];

        if(regs->ebx != entry->signature_ebx && entry->signature_ebx != SIGNATURE_ANY) {
            continue;
        }

        if(regs->ecx != entry->signature_ecx && entry->signature_ecx != SIGNATURE_ANY) {
            continue;
        }

        if(regs->edx != entry->signature_edx && entry->signature_edx != SIGNATURE_ANY) {
            continue;
        }

        return mapping[idx].id;
    }
}

/**
 * Identify the CPU vendor based on CPUID results
 * 
 * @param basic0 CPUID leaf 0
 * @return CPU vendor
 */
static int identify_vendor(const x86_cpuid_regs_t *basic0) {
    static const cpuid_signature_t mapping[] = {
        {
            .id             = CPU_VENDOR_AMD,
            .signature_ebx  = CPUID_VENDOR_AMD_EBX,
            .signature_ecx  = CPUID_VENDOR_AMD_ECX,
            .signature_edx  = CPUID_VENDOR_AMD_EDX
        },
        {
            .id             = CPU_VENDOR_CENTAUR_VIA,
            .signature_ebx  = CPUID_VENDOR_CENTAUR_EBX,
            .signature_ecx  = CPUID_VENDOR_CENTAUR_ECX,
            .signature_edx  = CPUID_VENDOR_CENTAUR_EDX
        },
        {
            .id             = CPU_VENDOR_CYRIX,
            .signature_ebx  = CPUID_VENDOR_CYRIX_EBX,
            .signature_ecx  = CPUID_VENDOR_CYRIX_ECX,
            .signature_edx  = CPUID_VENDOR_CYRIX_EDX
        },
        {
            .id             = CPU_VENDOR_CYRIX,
            .signature_ebx  = CPUID_VENDOR_GEODE_BY_NSC_EBX,
            .signature_ecx  = CPUID_VENDOR_GEODE_BY_NSC_ECX,
            .signature_edx  = CPUID_VENDOR_GEODE_BY_NSC_EDX
        },
        {
            .id             = CPU_VENDOR_HYGON,
            .signature_ebx  = CPUID_VENDOR_HYGON_EBX,
            .signature_ecx  = CPUID_VENDOR_HYGON_ECX,
            .signature_edx  = CPUID_VENDOR_HYGON_EDX
        },
        {
            .id             = CPU_VENDOR_INTEL,
            .signature_ebx  = CPUID_VENDOR_INTEL_EBX,
            .signature_ecx  = CPUID_VENDOR_INTEL_ECX,
            .signature_edx  = CPUID_VENDOR_INTEL_EDX
        },
        {
            .id             = CPU_VENDOR_ZHAOXIN,
            .signature_ebx  = CPUID_VENDOR_ZHAOXIN_EBX,
            .signature_ecx  = CPUID_VENDOR_ZHAOXIN_ECX,
            .signature_edx  = CPUID_VENDOR_ZHAOXIN_EDX
        },
        {
            .id             = CPU_VENDOR_GENERIC,
            .signature_ebx  = SIGNATURE_ANY,
            .signature_ecx  = SIGNATURE_ANY,
            .signature_edx  = SIGNATURE_ANY
        }
    };

    return map_signature(basic0, mapping);
}

/**
 * Detect CPU vendor and features
 * 
 * @param bootinfo boot information structure
 * @return CPU vendor
 */
void detect_cpu_features(bootinfo_t *bootinfo) {
    bootinfo->features      = 0;
    bootinfo->cpu_vendor    = CPU_VENDOR_GENERIC;

    bool has_cpuid = detect_cpuid();

    if(!has_cpuid) {
        return;
    }

    bootinfo->features |= BOOTINFO_FEATURE_CPUID;

    x86_cpuid_regs_t basic0;
    basic0.eax = 0;
    cpuid(&basic0);

    bootinfo->cpu_vendor= identify_vendor(&basic0);

    /* Check basic leaf 1 is supported. */
    if(basic0.eax < 1) {
        return;
    }

    x86_cpuid_regs_t basic1;
    basic1.eax = 1;
    cpuid(&basic1);

    bool has_pae = !!(basic1.edx & CPUID_FEATURE_PAE);

    if(!has_pae) {
        return;
    }

    bootinfo->features |= BOOTINFO_FEATURE_PAE;

    switch(bootinfo->cpu_vendor) {
        case CPU_VENDOR_AMD:
        /* See "VIA C7 in nanoBGA2 Datasheet" section 2.3.3:
         *
         * " Processor Signature and Feature Flags (EAX==0x80000001)
         *   Returns processor version information in EAX and Extended CPUID
         *   feature flags in EDX. EDX bit 20 indicates NoExecute support.
         *   NoExecute is used in Windows XP SP2 for virus protection. "
         * 
         * The "VIA C3 Nehemiah Processor Datasheet" section 2.3.2 (and
         * specifically table 3-3) shows the meaning of bit 6 of the
         * standard feature flags as "Physical Address Extension" but
         * indicates its value to be zero. This means we don't reach this
         * point in the execution for that earlier model. */
        case CPU_VENDOR_CENTAUR_VIA:
        /* Since the Hygon Dhyana is derived from the AMD Epyc, I think it is
         * safe to assume the NX bit is supported and that this is reflected
         * in the extended feature flags. I haven't tested this and don't have
         * documentation. */
        case CPU_VENDOR_HYGON:
        case CPU_VENDOR_INTEL:
        /* Since Zhaoxin processors are designs derived from Centaur/Via
         * designs, I think it is safe to assume the NX bit is supported and
         * that this is reflected in the extended feature flags. I haven't
         * tested this and don't have documentation. */
        case CPU_VENDOR_ZHAOXIN:
            break;
        default:
            return;
    }

    x86_cpuid_regs_t extended0;
    extended0.eax = 0x80000000;
    cpuid(&extended0);

    /* Check extended leaf 1 is supported. */
    if((extended0.eax & 0xffff0000) != 0x80000000 || extended0.eax < 0x80000001) {
        return;
    }

    x86_cpuid_regs_t extended1;
    extended1.eax = 0x80000001;
    cpuid(&extended1);

    bool has_nx = !!(extended1.edx & CPUID_EXT_FEATURE_NX);

    if(!has_nx) {
        return;
    }

    bootinfo->features |= BOOTINFO_FEATURE_NX;
}
