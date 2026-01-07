/*
 * Copyright (C) 2019-2026 Philippe Aubertin.
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
#include <kernel/infrastructure/i686/isa/cpuid.h>
#include <kernel/infrastructure/i686/isa/instrs.h>
#include <kernel/infrastructure/i686/isa/regs.h>
#include <kernel/infrastructure/i686/cpuidsig.h>
#include <kernel/infrastructure/i686/cpuinfo.h>
#include <kernel/interface/i686/bootinfo.h>
#include <kernel/machine/cpuinfo.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

/** information regarding the boot CPU */
static cpuinfo_t bsp_cpuinfo;

/** structure that represents all CPUID leafs supported by this kernel */
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
 * Update CPU information with features enumerated by setup code
 * 
 * @param cpuinfo structure in which to set the feature flags (OUT)
 * @param bootinfo boot information structure
 */
static void enumerate_bootinfo_features(cpuinfo_t *cpuinfo, const bootinfo_t *bootinfo) {
    cpuinfo->vendor     = bootinfo->cpu_vendor;
    cpuinfo->features   = 0;

    if(bootinfo_has_feature(bootinfo, BOOTINFO_FEATURE_CPUID)) {
        cpuinfo->features |= CPU_FEATURE_CPUID;
    }

    if(bootinfo_has_feature(bootinfo, BOOTINFO_FEATURE_PAE)) {
        cpuinfo->features |= CPU_FEATURE_PAE;
    }

    if(bootinfo_has_feature(bootinfo, BOOTINFO_FEATURE_NX)) {
        cpuinfo->features |= CPU_FEATURE_NX;
    }
}

/**
 * Fill the provided leafs structure by calling the CPUID instruction
 * 
 * @param leafs CPUID leafs structure (OUT)
 */
static void get_cpuid_leafs(cpuid_leafs_set *leafs) {
    memset(leafs, 0, sizeof(cpuid_leafs_set));

    if(!cpu_has_feature(CPU_FEATURE_CPUID)) {
        return;
    }

    const uint32_t ext_base = 0x80000000;
    const uint32_t soft_base = 0x40000000;
    
    leafs->basic0.eax = 0;
    leafs->basic1.eax = 1;
    leafs->ext0.eax = ext_base;

    /* leaf 0x00000000 */

    uint32_t basic_max = cpuid(&leafs->basic0);

    if(basic_max < 1) {
        /* This will lead to a kernel panic in check_cpu_minimum_requirements()
         * because CPU family will be zero. */
        error("CPUID function 1 is not supported.");
        return;
    }

    /* leaf 0x000000001 */

    (void)cpuid(&leafs->basic1);

    /* leaf 0x80000000 */

    uint32_t ext_max = cpuid(&leafs->ext0);

    if((ext_max & 0xffff0000) != 0x80000000) {
        ext_max = 0;
    }

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

    if(cpuinfo->family == 0xf || cpuinfo->family == 6) {
        uint8_t extended_model = (signature >> 16) & 0xf;
        cpuinfo->model += (extended_model << 4);
    }

    if(cpuinfo->family == 0xf) {
        uint8_t extended_family = (signature >> 20) & 0xff;
        cpuinfo->family += extended_family;
    }
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

    /* See the description of the SYSENTER instruction in the Intel® 64 and
     * IA-32 Architectures Software Developer’s Manual Volume 2:
     * 
     * " An operating system that qualifies the SEP flag must also qualify the
     *   processor family and model to ensure that the SYSENTER/SYSEXIT
     *   instructions are actually present. For example:
     *      IF CPUID SEP bit is set
     *          THEN IF (Family = 6) and (Model < 3) and (Stepping < 3)
     *              THEN
     *                  SYSENTER/SYSEXIT_Not_Supported; FI;
     *              ELSE
     *                  SYSENTER/SYSEXIT_Supported; FI;
     *      FI;
     * 
     *   When the CPUID instruction is executed on the Pentium Pro processor
     *   (model 1), the processor returns a the SEP flag as set, but does not
     *   support the SYSENTER/SYSEXIT instructions. "
     */
    bool is_intel = cpuinfo->vendor == CPU_VENDOR_INTEL;
    
    if(is_intel && cpuinfo->family == 6 && cpuinfo->model < 3 && cpuinfo->stepping < 3) {
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
    /* The SYSCALL/SYSRET instructions were defined as part of AMD64 and are
     * supported in (64-bit) long mode by all processors that support long
     * mode.
     * 
     * AMD processors that support these instructions support them in (32-bit)
     * protected mode as well whereas Intel processors only support them in
     * long mode.
     * 
     * I think it is safe to assume Hygon processors behave the same as AMD
     * since they are derived from AMD designs but I haven't tested this and
     * haven't seen relevant documentation. */
    if(cpuinfo->vendor != CPU_VENDOR_AMD && cpuinfo->vendor != CPU_VENDOR_HYGON) {
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

    /* support for local APIC */
    if(flags & CPUID_FEATURE_APIC) {
       cpuinfo->features |= CPU_FEATURE_APIC;
    }

    /* global pages */
    if(flags & CPUID_FEATURE_PGE) {
        cpuinfo->features |= CPU_FEATURE_PGE;
    }

    detect_sysenter_instruction(cpuinfo, leafs);

    detect_syscall_instruction(cpuinfo, leafs);
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
            .signature_ebx  = CPUID_SIGNATURE_ANY,
            .signature_ecx  = CPUID_SIGNATURE_ANY,
            .signature_edx  = CPUID_SIGNATURE_ANY
        }
    };

    if(! leafs->soft0_valid) {
        cpuinfo->hypervisor = HYPERVISOR_ID_NONE;
    } else {
        cpuinfo->hypervisor = map_cpuid_signature(&leafs->soft0, mapping);
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
 * Identify the number of bits of physical addresses
 * 
 * @param cpuinfo structure in which to set the number of address bits (OUT)
 * @param leafs CPUID leafs structure filled by a call to get_cpuid_leafs()
 */
static void identify_maxphyaddr(cpuinfo_t *cpuinfo, const cpuid_leafs_set *leafs) {
    if(!(cpuinfo->features & CPU_FEATURE_PAE)) {
        cpuinfo->maxphyaddr = 32;
    } else if(!leafs->ext8_valid) {
        cpuinfo->maxphyaddr = 36;
    } else {
        cpuinfo->maxphyaddr = leafs->ext8.eax & 0xff;
    }
}

/**
 * Log a string representation of the CPU feature flags
 * 
 * @param cpuinfo CPU information structure
 */
static void dump_features(const cpuinfo_t *cpuinfo) {
    info(
        "  Features:%s%s%s%s%s%s%s%s",
        (cpuinfo->features == 0) ? " (none)" : "",
        (cpuinfo->features & CPU_FEATURE_APIC) ? " apic" : "",
        (cpuinfo->features & CPU_FEATURE_CPUID) ? " cpuid" : "",
        (cpuinfo->features & CPU_FEATURE_NX) ? " nx" : "",
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
        case CPU_VENDOR_AMD:
            return "AMD";
        case CPU_VENDOR_CENTAUR_VIA:
            return "Centaur/Via";
        case CPU_VENDOR_CYRIX:
            return "Cyrix";
        case CPU_VENDOR_HYGON:
            return "Hygon";
        case CPU_VENDOR_INTEL:
            return "Intel";
        case CPU_VENDOR_GENERIC:
            return "Generic";
        default:
            return "???";
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
void detect_cpu_features(const bootinfo_t *bootinfo) {
    enumerate_bootinfo_features(&bsp_cpuinfo, bootinfo);

    cpuid_leafs_set cpuid_leafs;
    get_cpuid_leafs(&cpuid_leafs);

    identify_model(&bsp_cpuinfo, &cpuid_leafs);

    enumerate_features(&bsp_cpuinfo, &cpuid_leafs);
    
    identify_hypervisor(&bsp_cpuinfo, &cpuid_leafs);

    get_brand_string(&bsp_cpuinfo, &cpuid_leafs);

    identify_dcache_alignment(&bsp_cpuinfo, &cpuid_leafs);

    identify_maxphyaddr(&bsp_cpuinfo, &cpuid_leafs);

    dump_cpu_info(&bsp_cpuinfo);
}

/**
 * Check the CPU satisfies the minimum requirements for this kernel
 * 
 * This function panics if the minimum requirements aren't met. It is a
 * separate function because we want to defer this check and possible kernel
 * panic to after logging has been enabled.
 */
void check_cpu_minimum_requirements(void) {
    bool too_old = false;

    if(!cpu_has_feature(CPU_FEATURE_CPUID)) {
        error("CPUID instruction is not supported");
        too_old = true;
    } else if(bsp_cpuinfo.family < 5) {
        error("CPU family: %u", bsp_cpuinfo.family);
        too_old = true;
    }

    if(!cpu_has_feature(CPU_FEATURE_APIC)) {
        error("no integrated local APIC");
        too_old = true;
    }

    if(too_old) {
        panic("A Pentium CPU or later with an integrated local APIC is required");
    }
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
