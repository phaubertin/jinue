/*
 * Copyright (C) 2019-2025 Philippe Aubertin.
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
#include <ctype.h>
#include <stdio.h>
#include <string.h>

static cpuinfo_t bsp_cpuinfo;

typedef struct {
    x86_cpuid_regs_t    basic0;
    x86_cpuid_regs_t    basic1;
    x86_cpuid_regs_t    ext0;
    x86_cpuid_regs_t    ext1;
    x86_cpuid_regs_t    ext2;
    x86_cpuid_regs_t    ext3;
    x86_cpuid_regs_t    ext4;
    x86_cpuid_regs_t    ext8;
    x86_cpuid_regs_t    soft0;
    bool                ext4_valid;
    bool                ext8_valid;
    bool                soft0_valid;
} cpuid_leafs_set;

/**
 * Report CPU is too old to be supported and panic
 * 
 * This function never returns.
 */
static void too_old(void) {
    panic("A Pentium CPU or later with an integrated local APIC is required");
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
 * Fill the provided leafs structure by calling the CPUID instruction
 * 
 * @param leafs CPUID leafs structure (OUT)
 */
static void get_cpuid_leafs(cpuid_leafs_set *leafs) {
    memset(leafs, 0, sizeof(cpuid_leafs_set));

    const uint32_t ext_base = 0x80000000;
    const uint32_t soft_base = 0x40000000;
    
    leafs->basic0.eax = 0;
    leafs->basic1.eax = 1;
    leafs->ext0.eax = ext_base;

    /* leaf 0x00000000 */

    uint32_t basic_max = cpuid(&leafs->basic0);

    if(basic_max < 1) {
        error("CPUID function 1 is not supported");
        too_old();
    }

    /* leaf 0x000000001 */

    (void)cpuid(&leafs->basic1);

    /* leaf 0x80000000 */

    uint32_t ext_max = cpuid(&leafs->ext0);

    /* leaf 0x80000001 */

    if(ext_max >= ext_base + 1) {
        leafs->ext1.eax = ext_base + 1;
        (void)cpuid(&leafs->ext1);
    }

    /* leafs 0x80000002-0x80000004 (CPU brand string) */

    leafs->ext4_valid = ext_max >= ext_base + 4;

    if(leafs->ext4_valid) {
        leafs->ext2.eax = ext_base + 2;
        leafs->ext3.eax = ext_base + 3;
        leafs->ext4.eax = ext_base + 4;
        (void)cpuid(&leafs->ext2);
        (void)cpuid(&leafs->ext3);
        (void)cpuid(&leafs->ext4);
    }

    /* leaf 0x80000008 */

    leafs->ext8_valid = ext_max >= ext_base + 8;

    if(leafs->ext8_valid) {
        leafs->ext8.eax = ext_base + 8;
        (void)cpuid(&leafs->ext8);
    }

    /* leaf 0x40000000 (software/hypervisor) */

    leafs->soft0.eax = soft_base;
    (void)cpuid(&leafs->soft0);

    /* Regarding KVM:
     *
     * " Note also that old hosts set eax value to 0x0. This should be
     *   interpreted as if the value was 0x40000001. "
     *
     * https://docs.kernel.org/virt/kvm/x86/cpuid.html */
    bool needs_kvm_eax_fix =
           leafs->soft0.eax == 0
        && leafs->soft0.ebx == CPUID_HYPERVISOR_KVM_EBX
        && leafs->soft0.ecx == CPUID_HYPERVISOR_KVM_ECX
        && leafs->soft0.edx == CPUID_HYPERVISOR_KVM_EDX;

    if(needs_kvm_eax_fix) {
        leafs->soft0.eax = soft_base + 1;
    }

    uint32_t soft0_eax = leafs->soft0.eax;
    leafs->soft0_valid = soft0_eax >= soft_base && soft0_eax < soft_base + 0x10000000;
}

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
 * @param cpuinfo structure in which to set the vendor (OUT)
 * @param leafs CPUID leafs structure filled by a call to get_cpuid_leafs()
 */
static void identify_vendor(cpuinfo_t *cpuinfo, const cpuid_leafs_set *leafs) {
    static const cpuid_signature_t mapping[] = {
        {
            .id             = CPUINFO_VENDOR_AMD,
            .signature_ebx  = CPUID_VENDOR_AMD_EBX,
            .signature_ecx  = CPUID_VENDOR_AMD_ECX,
            .signature_edx  = CPUID_VENDOR_AMD_EDX
        },
        {
            .id             = CPUINFO_VENDOR_INTEL,
            .signature_ebx  = CPUID_VENDOR_INTEL_EBX,
            .signature_ecx  = CPUID_VENDOR_INTEL_ECX,
            .signature_edx  = CPUID_VENDOR_INTEL_EDX
        },
        {
            .id             = CPUINFO_VENDOR_GENERIC,
            .signature_ebx  = SIGNATURE_ANY,
            .signature_ecx  = SIGNATURE_ANY,
            .signature_edx  = SIGNATURE_ANY
        }
    };

    cpuinfo->vendor = map_signature(&leafs->basic0, mapping);
}

/**
 * Identify the hypervisor
 * 
 * @param cpuinfo structure in which to set the hypervisor ID (OUT)
 * @param leafs CPUID leafs structure filled by a call to get_cpuid_leafs()
 */
static void identify_hypervisor(cpuinfo_t *cpuinfo, const cpuid_leafs_set *leafs) {
    static const cpuid_signature_t mapping[] = {
        {
            .id             = HYPERVISOR_ID_ACRN,
            .signature_ebx  = CPUID_HYPERVISOR_ACRN,
            .signature_ecx  = CPUID_HYPERVISOR_ACRN,
            .signature_edx  = CPUID_HYPERVISOR_ACRN
        },
        {
            .id             = HYPERVISOR_ID_BHYVE,
            .signature_ebx  = CPUID_HYPERVISOR_BHYVE_EBX,
            .signature_ecx  = CPUID_HYPERVISOR_BHYVE_ECX,
            .signature_edx  = CPUID_HYPERVISOR_BHYVE_EDX
        },
        {
            .id             = HYPERVISOR_ID_KVM,
            .signature_ebx  = CPUID_HYPERVISOR_KVM_EBX,
            .signature_ecx  = CPUID_HYPERVISOR_KVM_ECX,
            .signature_edx  = CPUID_HYPERVISOR_KVM_EDX
        },
        {
            .id             = HYPERVISOR_ID_QEMU,
            .signature_ebx  = CPUID_HYPERVISOR_QEMU_EBX,
            .signature_ecx  = CPUID_HYPERVISOR_QEMU_ECX,
            .signature_edx  = CPUID_HYPERVISOR_QEMU_EDX
        },
        {
            .id             = HYPERVISOR_ID_VMWARE,
            .signature_ebx  = CPUID_HYPERVISOR_VMWARE_EBX,
            .signature_ecx  = CPUID_HYPERVISOR_VMWARE_ECX,
            .signature_edx  = CPUID_HYPERVISOR_VMWARE_EDX
        },
        {
            .id             = HYPERVISOR_ID_XEN,
            .signature_ebx  = CPUID_HYPERVISOR_XEN_EBX,
            .signature_ecx  = CPUID_HYPERVISOR_XEN_ECX,
            .signature_edx  = CPUID_HYPERVISOR_XEN_EDX
        },
        {
            .id             = HYPERVISOR_ID_UNKNOWN,
            .signature_ebx  = SIGNATURE_ANY,
            .signature_ecx  = SIGNATURE_ANY,
            .signature_edx  = SIGNATURE_ANY
        }
    };

    if(! leafs->soft0_valid) {
        cpuinfo->hypervisor = HYPERVISOR_ID_NONE;
    } else {
        cpuinfo->hypervisor = map_signature(&leafs->soft0, mapping);
    }
}

/**
 * Identify the CPU family, model and stepping
 * 
 * @param cpuinfo structure in which to set the information (OUT)
 * @param leafs CPUID leafs structure filled by a call to get_cpuid_leafs()
 */
static void identify_model(cpuinfo_t *cpuinfo, const cpuid_leafs_set *leafs) {
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

typedef enum {
    CLEAN_STATE_START,
    CLEAN_STATE_IDLE,
    CLEAN_STATE_SPACE,
} clean_state_t;

/**
 * Clean the CPU brand string
 * 
 * Remove leading and trailing whitespace characters, coalesce sequences of
 * whitespace charaters and remove non-printable characters.
 * 
 * @param buffer string buffer containing brand string
 */
static void clean_brand_string(char *buffer) {
    clean_state_t state = CLEAN_STATE_START;

    int dest = 0;
    int src = 0;

    /* Invariant: dest is always at or before src, so what happens at dest
     * never affects the condition of this loop. */
    while(true) {
        int c = buffer[src];

        if(c == '\0') {
            /* Add a single space character if there is a pending space. This
             * ensure space/tab sequences are coalesced into a single space
             * charater and leading spaces are trimmed. */
            if(state == CLEAN_STATE_SPACE) {
                buffer[dest++] = ' ';
            }

            buffer[dest++] = '\0';
            break;
        } else if(isblank(c)) {
            /* Transition to the space state only if at least one printable
             * character was encountered since the start of the string. */
            if(state == CLEAN_STATE_IDLE) {
                state = CLEAN_STATE_SPACE;
            }
        } else if(isprint(c) && !isspace(c)) {
            if(state == CLEAN_STATE_SPACE) {
                buffer[dest++] = ' ';
            }

            buffer[dest++] = c;
            state = CLEAN_STATE_IDLE;
        }

        ++src;
    }
}

/**
 * Get the CPU brand string
 * 
 * Sets an empty string if the brand string cannot be retrieved.
 * 
 * @param cpuinfo structure in which to set the brand string (OUT)
 * @param leafs CPUID leafs structure filled by a call to get_cpuid_leafs()
 */
static void get_brand_string(cpuinfo_t *cpuinfo, const cpuid_leafs_set *leafs) {
    if(! leafs->ext4_valid) {
        cpuinfo->brand_string[0] = '\0';
        return;
    }

    snprintf(
        cpuinfo->brand_string,
        sizeof(cpuinfo->brand_string),
        "%.4s%.4s%.4s%.4s%.4s%.4s%.4s%.4s%.4s%.4s%.4s%.4s",
        (const char *)&leafs->ext2.eax,
        (const char *)&leafs->ext2.ebx,
        (const char *)&leafs->ext2.ecx,
        (const char *)&leafs->ext2.edx,
        (const char *)&leafs->ext3.eax,
        (const char *)&leafs->ext3.ebx,
        (const char *)&leafs->ext3.ecx,
        (const char *)&leafs->ext3.edx,
        (const char *)&leafs->ext4.eax,
        (const char *)&leafs->ext4.ebx,
        (const char *)&leafs->ext4.ecx,
        (const char *)&leafs->ext4.edx
    );

    clean_brand_string(cpuinfo->brand_string);
}

/**
 * Identify the data cache alignment
 * 
 * Defaults to 32 if the data cache alignment is not reported by the CPUID instruction.
 * 
 * @param cpuinfo structure in which to set the cache alignment (OUT)
 * @param leafs CPUID leafs structure filled by a call to get_cpuid_leafs()
 */
static void identify_dcache_alignment(cpuinfo_t *cpuinfo, const cpuid_leafs_set *leafs) {
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
 * Sets the CPU_FEATURE_SYSENTER feature flag if the SYSENTER instruction is
 * supported.
 * 
 * @param cpuinfo structure in which to set the feature flag (OUT)
 * @param leafs CPUID leafs structure filled by a call to get_cpuid_leafs()
 */
static void detect_sysenter_instruction(cpuinfo_t *cpuinfo, const cpuid_leafs_set *leafs) {
    if(!(leafs->basic1.edx & CPUID_FEATURE_SEP)) {
        return;
    }

    if(is_intel(cpuinfo) && cpuinfo->family == 6 && cpuinfo->model < 3 && cpuinfo->stepping < 3) {
        return;
    }

    cpuinfo->features |= CPU_FEATURE_SYSENTER;
}

/**
 * Detect whether the CPU supports the SYSCALL fast system call instruction
 * 
 * Sets the CPU_FEATURE_SYSCALL feature flag if the SYSCALL instruction is
 * supported.
 * 
 * @param cpuinfo structure in which to set the feature flag (OUT)
 * @param leafs CPUID leafs structure filled by a call to get_cpuid_leafs()
 */
static void detect_syscall_instruction(cpuinfo_t *cpuinfo, const cpuid_leafs_set *leafs) {
    if(!is_amd(cpuinfo)) {
        return;
    }

    if(leafs->ext1.edx & CPUID_EXT_FEATURE_SYSCALL) {
        cpuinfo->features |= CPU_FEATURE_SYSCALL;
    }
}

/**
 * Enumerate CPU features
 * 
 * This function detects various features that may be supported by the CPU and
 * sets feature flag in the CPU information structure accordingly.
 * 
 * @param cpuinfo structure in which to set the feature flags (OUT)
 * @param leafs CPUID leafs structure filled by a call to get_cpuid_leafs()
 */
static void enumerate_features(cpuinfo_t *cpuinfo, const cpuid_leafs_set *leafs) {
    uint32_t flags = leafs->basic1.edx;
    uint32_t ext_flags = leafs->ext1.edx;

    /* support for local APIC */
    if(!(flags & CPUID_FEATURE_APIC)) {
        error("no integrated local APIC");
        too_old();
    }
    
    cpuinfo->features = 0;

    /* global pages */
    if(flags & CPUID_FEATURE_PGE) {
        cpuinfo->features |= CPU_FEATURE_PGE;
    }

    detect_sysenter_instruction(cpuinfo, leafs);

    detect_syscall_instruction(cpuinfo, leafs);

    if(!is_amd_or_intel(cpuinfo)) {
        return;
    }

    /* Enabling Physical Address Extension (PAE) and No-Execute (NX) bit */
    if((flags & CPUID_FEATURE_PAE) && (ext_flags & CPUID_FEATURE_NXE)) {
        cpuinfo->features |= CPU_FEATURE_PAE;
    }
}

/**
 * Identify the number of bits of physical addresses
 * 
 * @param cpuinfo structure in which to set the number of address bits (OUT)
 * @param leafs CPUID leafs structure filled by a call to get_cpuid_leafs()
 */
static void identify_maxphyaddr(cpuinfo_t *cpuinfo, const cpuid_leafs_set *leafs) {
    if((cpuinfo->features & CPU_FEATURE_PAE) && leafs->ext8_valid) {
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
    info(
        "  Features:%s%s%s%s",
        (cpuinfo->features & CPU_FEATURE_PAE) ? " pae" : "",
        (cpuinfo->features & CPU_FEATURE_PGE) ? " pge" : "",
        (cpuinfo->features & CPU_FEATURE_SYSCALL) ? " syscall" : "",
        (cpuinfo->features & CPU_FEATURE_SYSENTER) ? " sysenter" : ""
    );
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

static const char *get_hypervisor_string(const cpuinfo_t *cpuinfo) {
    switch(cpuinfo->hypervisor) {
        case HYPERVISOR_ID_ACRN:
            return "ACRN";
        case HYPERVISOR_ID_BHYVE:
            return "bhyve";
        case HYPERVISOR_ID_HYPER_V:
            return "Microsoft Hyper-V";
        case HYPERVISOR_ID_KVM:
            return "Linux KVM";
        case HYPERVISOR_ID_QEMU:
            return "QEMU TCG";
        case HYPERVISOR_ID_VMWARE:
            return "VMware";
        case HYPERVISOR_ID_XEN:
            return "Xen";
        default:
            return "Unknown";
    }
}

/**
 * Log the contents of the CPU information structure
 * 
 * @param cpuinfo CPU information structure
 */
static void dump_cpu_info(const cpuinfo_t *cpuinfo) {
    info("CPU information:");

    info(
        "  Vendor: %s family: %u model: %u stepping: %u",
        get_vendor_string(cpuinfo),
        cpuinfo->family,
        cpuinfo->model,
        cpuinfo->stepping
    );
    
    dump_features(cpuinfo);

    info("  Brand string: %s", cpuinfo->brand_string);
    info("  Data cache alignment: %u bytes", cpuinfo->dcache_alignment);
    info("  Physical address size: %u bits", cpuinfo->maxphyaddr);

    if(cpuinfo->hypervisor == HYPERVISOR_ID_NONE) {
        info("No virtualization");
    } else {
        info("Virtualization environment: %s", get_hypervisor_string(cpuinfo));
    }
}

/**
 * Detect the features of the bootstrap processor (BSP)
 */
void detect_cpu_features(void) {
    check_cpuid_is_supported();

    cpuid_leafs_set cpuid_leafs;
    get_cpuid_leafs(&cpuid_leafs);
    
    identify_vendor(&bsp_cpuinfo, &cpuid_leafs);

    identify_hypervisor(&bsp_cpuinfo, &cpuid_leafs);

    identify_model(&bsp_cpuinfo, &cpuid_leafs);

    get_brand_string(&bsp_cpuinfo, &cpuid_leafs);

    identify_dcache_alignment(&bsp_cpuinfo, &cpuid_leafs);

    enumerate_features(&bsp_cpuinfo, &cpuid_leafs);

    identify_maxphyaddr(&bsp_cpuinfo, &cpuid_leafs);

    dump_cpu_info(&bsp_cpuinfo);
}

/**
 * Get the data cache alignment of the BSP
 * 
 * @return data cache alignment in bytes
 */
unsigned int machine_get_cpu_dcache_alignment(void) {
    return bsp_cpuinfo.dcache_alignment;
}

/**
 * Determine whether the CPU supports the specified feature
 * 
 * Use the CPU_FEATURE_... constants for the mask arguments. A bitwise or of
 * multiple of these constants is allowed, in which case this function will
 * return true only if all the specified features are supported.
 * 
 * This function returns whether the boot CPU supports the specified feature.
 * However, all CPUs should support the same feature set.
 * 
 * @param mask feature(s) for which to check support
 * @return true if feature is supported, false otherwise
 */
bool cpu_has_feature(uint32_t mask) {
    return (bsp_cpuinfo.features & mask) == mask;
}

/**
 * Determine the width of physical addresses in bits
 * 
 * This function returns the information for the boot CPU. However, all CPUs
 * should support the same feature set.
 * 
 * @return the width of physical addresses in bits
 */
unsigned int cpu_phys_addr_width(void) {
    return bsp_cpuinfo.maxphyaddr;
}
