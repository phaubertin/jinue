/*
 * Copyright (C) 2019-2024 Philippe Aubertin.
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
#include <kernel/domain/services/logging.h>
#include <kernel/domain/services/panic.h>
#include <kernel/infrastructure/i686/asm/cpuid.h>
#include <kernel/infrastructure/i686/asm/eflags.h>
#include <kernel/infrastructure/i686/isa/instrs.h>
#include <kernel/infrastructure/i686/isa/regs.h>
#include <kernel/infrastructure/i686/cpuinfo.h>
#include <kernel/machine/cpuinfo.h>
#include <stdio.h>
#include <string.h>

cpuinfo_t bsp_cpuinfo;

/**
 * Report CPU is too old to be supported and panic
 * 
 * This function never returns.
 */
static void too_old(void) {
    panic("Pentium CPU or later is required");
}

/**
 * Check whether the CPUID instruction is supported, panic otherwise
 */
static void check_cpuid_is_supported(void) {
    /* The CPUID instruction is available if we can change the value of eflags
     * bit 21 (ID) */
    uint32_t temp_eflags = get_eflags();
    temp_eflags ^= EFLAGS_ID;
    set_eflags(temp_eflags);
    
    if((temp_eflags & EFLAGS_ID) != (get_eflags() & EFLAGS_ID)) {
        error("CPUID instruction is not supported");
        too_old();
    }
}

/**
 * Call the CPUID instruction and fill the provided CPUID leafs structure
 * 
 * @param leafs CPUID leafs structure (OUT)
 */
static void call_cpuid(x86_cpuid_leafs *leafs) {
    memset(leafs, 0, sizeof(x86_cpuid_leafs));

    const uint32_t ext_base = 0x80000000;
    
    leafs->basic0.eax = 0;
    leafs->basic1.eax = 1;
    leafs->ext0.eax = ext_base;

    uint32_t basic_max = cpuid(&leafs->basic0);

    if(basic_max < 1) {
        error("CPUID function 1 is not supported");
        too_old();
    }

    (void)cpuid(&leafs->basic1);

    uint32_t ext_max = cpuid(&leafs->ext0);

    if(ext_max >= ext_base + 1) {
        leafs->ext1.eax = ext_base + 1;
        (void)cpuid(&leafs->ext1);
    }

    leafs->ext8_valid = ext_max >= ext_base + 8;

    if(leafs->ext8_valid) {
        leafs->ext8.eax = ext_base + 8;
        (void)cpuid(&leafs->ext8);
    }
}

typedef struct {
    int         vendor;
    uint32_t    vendor_dw0;
    uint32_t    vendor_dw1;
    uint32_t    vendor_dw2;
} cpuid_vendor_t;

static const cpuid_vendor_t cpuid_vendors[] = {
    {
        .vendor     = CPUINFO_VENDOR_AMD,
        .vendor_dw0 = CPUID_VENDOR_AMD_DW0,
        .vendor_dw1 = CPUID_VENDOR_AMD_DW1,
        .vendor_dw2 = CPUID_VENDOR_AMD_DW2
    },
    {
        .vendor     = CPUINFO_VENDOR_INTEL,
        .vendor_dw0 = CPUID_VENDOR_INTEL_DW0,
        .vendor_dw1 = CPUID_VENDOR_INTEL_DW1,
        .vendor_dw2 = CPUID_VENDOR_INTEL_DW2
    }
};

/**
 * Identify the CPU vendor based on CPUID results
 * 
 * @param cpuinfo structure in which to set the vendor (OUT)
 * @param leafs CPUID leafs structure filled by a call_cpuid()
 */
static void identify_vendor(cpuinfo_t *cpuinfo, const x86_cpuid_leafs *leafs) {
    const x86_cpuid_regs_t *regs = &leafs->basic0;
    
    for(int idx = 0; idx < sizeof(cpuid_vendors) / sizeof(cpuid_vendors[0]); ++idx) {
        if(regs->ebx != cpuid_vendors[idx].vendor_dw0) {
            continue;
        }

        if(regs->edx != cpuid_vendors[idx].vendor_dw1) {
            continue;
        }

        if(regs->ecx != cpuid_vendors[idx].vendor_dw2) {
            continue;
        }

        cpuinfo->vendor = cpuid_vendors[idx].vendor;
        return;
    }

    cpuinfo->vendor = CPUINFO_VENDOR_GENERIC;
}

/**
 * Identify the CPU family, model and stepping
 * 
 * @param cpuinfo structure in which to set the information (OUT)
 * @param leafs CPUID leafs structure filled by a call_cpuid()
 */
static void identify_model(cpuinfo_t *cpuinfo, const x86_cpuid_leafs *leafs) {
    uint32_t signature   = leafs->basic1.eax;
    cpuinfo->family      = (signature>>8)  & 0xf;
    cpuinfo->model       = (signature>>4)  & 0xf;
    cpuinfo->stepping    = signature       & 0xf;

    /* The signature format is different for 386 processors. However 1) the
     * family field is the same in both format and 2) this check ensures the
     * format is the one we want. */
    if(cpuinfo->family < 5) {
        error("CPU family: %u", cpuinfo->family);
        too_old();
    }
}

/**
 * Identify the data cache alignment
 * 
 * Defaults to 32 if the data cache alignment is not reported by the CPUID instruction.
 * 
 * @param cpuinfo structure in which to set the cache alignment (OUT)
 * @param leafs CPUID leafs structure filled by a call_cpuid()
 */
static void identify_dcache_alignment(cpuinfo_t *cpuinfo, const x86_cpuid_leafs *leafs) {
    const x86_cpuid_regs_t *regs = &leafs->basic1;

    if(regs->edx & CPUID_FEATURE_CLFLUSH) {
        cpuinfo->dcache_alignment = ((regs->ebx >> 8) & 0xff) * 8;
    } else {
        cpuinfo->dcache_alignment = 32;
    }
}

/**
 * Utility function that determines whether CPU vendor is AMD
 * 
 * @param cpuinfo CPU information structure
 * @return true if CPU vendor is AMD, false otherwise
 */
static bool is_amd(const cpuinfo_t *cpuinfo) {
    return cpuinfo->vendor == CPUINFO_VENDOR_AMD;
}

/**
 * Utility function that determines whether CPU vendor is Intel
 * 
 * @param cpuinfo CPU information structure
 * @return true if CPU vendor is Intel, false otherwise
 */
static bool is_intel(const cpuinfo_t *cpuinfo) {
    return cpuinfo->vendor == CPUINFO_VENDOR_INTEL;
}

/**
 * Utility function that determines whether CPU vendor is AMD or Intel
 * 
 * @param cpuinfo CPU information structure
 * @return true if CPU vendor is AMD or Intel, false otherwise
 */
static bool is_amd_or_intel(const cpuinfo_t *cpuinfo) {
    return cpuinfo->vendor == CPUINFO_VENDOR_AMD || cpuinfo->vendor == CPUINFO_VENDOR_INTEL;
}

/**
 * Detect whether the CPU supports the SYSENTER fast system call instruction
 * 
 * Sets the CPUINFO_FEATURE_SYSENTER feature flag if the SYSENTER instruction
 * is supported.
 * 
 * @param cpuinfo structure in which to set the feature flag (OUT)
 * @param leafs CPUID leafs structure filled by a call_cpuid()
 */
static void detect_sysenter_instruction(cpuinfo_t *cpuinfo, const x86_cpuid_leafs *leafs) {
    if(!(leafs->basic1.edx & CPUID_FEATURE_SEP)) {
        return;
    }

    if(is_intel(cpuinfo) && cpuinfo->family == 6 && cpuinfo->model < 3 && cpuinfo->stepping < 3) {
        return;
    }

    cpuinfo->features |= CPUINFO_FEATURE_SYSENTER;
}

/**
 * Detect whether the CPU supports the SYSCALL fast system call instruction
 * 
 * Sets the CPUINFO_FEATURE_SYSCALL feature flag if the SYSCALL instruction
 * is supported.
 * 
 * @param cpuinfo structure in which to set the feature flag (OUT)
 * @param leafs CPUID leafs structure filled by a call_cpuid()
 */
static void detect_syscall_instruction(cpuinfo_t *cpuinfo, const x86_cpuid_leafs *leafs) {
    if(!is_amd(cpuinfo)) {
        return;
    }

    if(leafs->ext1.edx & CPUID_EXT_FEATURE_SYSCALL) {
        cpuinfo->features |= CPUINFO_FEATURE_SYSCALL;
    }
}

/**
 * Enumerate CPU features
 * 
 * This function detects various features that may be supported by the CPU and
 * sets feature flag in the CPU information structure accordingly.
 * 
 * @param cpuinfo structure in which to set the feature flags (OUT)
 * @param leafs CPUID leafs structure filled by a call_cpuid()
 */
static void enumerate_features(cpuinfo_t *cpuinfo, const x86_cpuid_leafs *leafs) {
    uint32_t flags = leafs->basic1.edx;
    uint32_t ext_flags = leafs->ext1.edx;
    
    cpuinfo->features = 0;

    /* global pages */
    if(flags & CPUID_FEATURE_PGE) {
        cpuinfo->features |= CPUINFO_FEATURE_PGE;
    }

    detect_sysenter_instruction(cpuinfo, leafs);

    detect_syscall_instruction(cpuinfo, leafs);

    if(!is_amd_or_intel(cpuinfo)) {
        return;
    }

    /* support for local APIC */
    if(flags & CPUID_FEATURE_APIC) {
        cpuinfo->features |= CPUINFO_FEATURE_LOCAL_APIC;
    }

    /* large 4MB pages in 32-bit (non-PAE) paging mode */
    if(flags & CPUID_FEATURE_PSE) {
        cpuinfo->features |= CPUINFO_FEATURE_PSE;
    }

    /* Enabling Physical Address Extension (PAE) and No-Execute (NX) bit */
    if((flags & CPUID_FEATURE_PAE) && (ext_flags & CPUID_FEATURE_NXE)) {
        cpuinfo->features |= CPUINFO_FEATURE_PAE;
    }
}

/**
 * Identify the number of bits of physical addresses
 * 
 * @param cpuinfo structure in which to set the number of address bits (OUT)
 * @param leafs CPUID leafs structure filled by a call_cpuid()
 */
static void identify_maxphyaddr(cpuinfo_t *cpuinfo, const x86_cpuid_leafs *leafs) {
    if((cpuinfo->features & CPUINFO_FEATURE_PAE) && leafs->ext8_valid) {
        cpuinfo->maxphyaddr = leafs->ext8.eax & 0xff;
    } else {
        cpuinfo->maxphyaddr = 32;
    }
}

/**
 * Log a string representation of the CPU feature flags
 * 
 * @param cpuinfo CPU information structure
 */
static void dump_features(const cpuinfo_t *cpuinfo) {
    char buffer[40];

    snprintf(
        buffer,
        sizeof(buffer),
        "%s%s%s%s%s%s",
        (cpuinfo->features & CPUINFO_FEATURE_LOCAL_APIC) ? " apic" : "",
        (cpuinfo->features & CPUINFO_FEATURE_PAE) ? " pae" : "",
        (cpuinfo->features & CPUINFO_FEATURE_PGE) ? " pge" : "",
        (cpuinfo->features & CPUINFO_FEATURE_PSE) ? " pse" : "",
        (cpuinfo->features & CPUINFO_FEATURE_SYSCALL) ? " syscall" : "",
        (cpuinfo->features & CPUINFO_FEATURE_SYSENTER) ? " sysenter" : ""
    );

    info("CPU features:%s", buffer);
}

/**
 * Return the name of the CPU vendor
 * 
 * @param cpuinfo CPU information structure
 * @return CPU vendor name string
 */
static const char *get_vendor_string(const cpuinfo_t *cpuinfo) {
    switch(cpuinfo->vendor) {
        case CPUINFO_VENDOR_AMD:
            return "AMD";
        case CPUINFO_VENDOR_INTEL:
            return "Intel";
        default:
            return "Generic";
    }
}

/**
 * Log the contents of the CPU information structure
 * 
 * @param cpuinfo CPU information structure
 */
static void dump_cpu_features(const cpuinfo_t *cpuinfo) {
    info(
        "CPU vendor: %s family: %u model: %u stepping: %u",
        get_vendor_string(cpuinfo),
        cpuinfo->family,
        cpuinfo->model,
        cpuinfo->stepping
    );
    
    dump_features(cpuinfo);

    info("CPU data cache alignment: %u bytes", cpuinfo->dcache_alignment);
    info("CPU physical address size: %u bits", cpuinfo->maxphyaddr);
}

/**
 * Detect the features of the bootstrap processor (BSP)
 */
void detect_cpu_features(void) {
    check_cpuid_is_supported();

    x86_cpuid_leafs cpuid_leafs;
    call_cpuid(&cpuid_leafs);
    
    identify_vendor(&bsp_cpuinfo, &cpuid_leafs);

    identify_model(&bsp_cpuinfo, &cpuid_leafs);

    identify_dcache_alignment(&bsp_cpuinfo, &cpuid_leafs);

    enumerate_features(&bsp_cpuinfo, &cpuid_leafs);

    identify_maxphyaddr(&bsp_cpuinfo, &cpuid_leafs);

    dump_cpu_features(&bsp_cpuinfo);
}

/**
 * Get the data cache alignment of the BSP
 * 
 * @return data cache alignment in bytes
 */
unsigned int machine_get_cpu_dcache_alignment(void) {
    return bsp_cpuinfo.dcache_alignment;
}
