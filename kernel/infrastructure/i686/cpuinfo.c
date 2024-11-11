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

#include <kernel/infrastructure/i686/asm/cpuid.h>
#include <kernel/infrastructure/i686/asm/eflags.h>
#include <kernel/infrastructure/i686/cpuinfo.h>
#include <kernel/infrastructure/i686/x86.h>
#include <kernel/machine/cpuinfo.h>

cpuinfo_t cpuinfo;

void detect_cpu_features(void) {
    uint32_t temp_eflags;
    
    /* default values */
    cpuinfo.maxphyaddr         = 32;
    cpuinfo.dcache_alignment   = 32;
    cpuinfo.features           = 0;
    cpuinfo.vendor             = CPUINFO_VENDOR_GENERIC;
    cpuinfo.family             = 0;
    cpuinfo.model              = 0;
    cpuinfo.stepping           = 0;
    
    /* The CPUID instruction is available if we can change the value of eflags
     * bit 21 (ID) */
    temp_eflags  = get_eflags();
    temp_eflags ^= CPU_EFLAGS_ID;
    set_eflags(temp_eflags);
    
    if(temp_eflags == get_eflags()) {
        cpuinfo.features |= CPUINFO_FEATURE_CPUID;
    }
    
    if(cpu_has_feature(CPUINFO_FEATURE_CPUID)) {
        x86_cpuid_regs_t regs;

        /* default values */
        uint32_t flags      = 0;
        uint32_t ext_flags  = 0;

        /* function 0: vendor ID string, max value of eax when calling CPUID */
        regs.eax = 0;
        
        /* call CPUID instruction */
        uint32_t cpuid_max  = cpuid(&regs);
        uint32_t vendor_dw0 = regs.ebx;
        uint32_t vendor_dw1 = regs.edx;
        uint32_t vendor_dw2 = regs.ecx;
        
        /* identify vendor */
        if(    vendor_dw0 == CPUID_VENDOR_AMD_DW0 
            && vendor_dw1 == CPUID_VENDOR_AMD_DW1
            && vendor_dw2 == CPUID_VENDOR_AMD_DW2) {
                
            cpuinfo.vendor = CPUINFO_VENDOR_AMD;
        }
        else if (vendor_dw0 == CPUID_VENDOR_INTEL_DW0
            &&   vendor_dw1 == CPUID_VENDOR_INTEL_DW1
            &&   vendor_dw2 == CPUID_VENDOR_INTEL_DW2) {
                
            cpuinfo.vendor = CPUINFO_VENDOR_INTEL;
        }
        
        /* get processor signature (family/model/stepping) and feature flags */
        if(cpuid_max >= 1) {
            /* function 1: processor signature and feature flags */
            regs.eax = 1;

            /* call CPUID instruction */
            uint32_t signature = cpuid(&regs);

            /* set processor signature */
            cpuinfo.stepping  = signature       & 0xf;
            cpuinfo.model     = (signature>>4)  & 0xf;
            cpuinfo.family    = (signature>>8)  & 0xf;

            /* feature flags */
            flags = regs.edx;

            /* cache alignment */
            if(flags & CPUID_FEATURE_CLFLUSH) {
                cpuinfo.dcache_alignment = ((regs.ebx >> 8) & 0xff) * 8;
            }

            /* global pages */
            if(flags & CPUID_FEATURE_PGE) {
                cpuinfo.features |= CPUINFO_FEATURE_PGE;
            }
        }

        /* extended function 0: max value of eax when calling CPUID (extended function) */
        regs.eax = 0x80000000;
        uint32_t cpuid_ext_max = cpuid(&regs);

        /* get extended feature flags */
        if(cpuid_ext_max >= 0x80000001) {
            /* extended function 1: extended feature flags */
            regs.eax = 0x80000001;
            (void)cpuid(&regs);

            /* extended feature flags */
            ext_flags = regs.edx;
        }

        /* support for SYSENTER/SYSEXIT instructions */
        if(flags & CPUID_FEATURE_SEP) {
            if(cpuinfo.vendor == CPUINFO_VENDOR_AMD) {
                cpuinfo.features |= CPUINFO_FEATURE_SYSENTER;
            }
            else if(cpuinfo.vendor == CPUINFO_VENDOR_INTEL) {
                if(cpuinfo.family == 6 && cpuinfo.model < 3 && cpuinfo.stepping < 3) {
                    /* not supported */
                }
                else {
                    cpuinfo.features |= CPUINFO_FEATURE_SYSENTER;
                }
            }
        }

        /* support for SYSCALL/SYSRET instructions */
        if(cpuinfo.vendor == CPUINFO_VENDOR_AMD) {
            if(ext_flags & CPUID_EXT_FEATURE_SYSCALL) {
                cpuinfo.features |= CPUINFO_FEATURE_SYSCALL;
            }
        }

        if(cpuinfo.vendor == CPUINFO_VENDOR_AMD || cpuinfo.vendor == CPUINFO_VENDOR_INTEL) {
            /* support for local APIC */
            if(flags & CPUID_FEATURE_APIC) {
                cpuinfo.features |= CPUINFO_FEATURE_LOCAL_APIC;
            }

            /* large 4MB pages in 32-bit (non-PAE) paging mode */
            if(flags & CPUID_FEATURE_PSE) {
                cpuinfo.features |= CPUINFO_FEATURE_PSE;
            }

            /* support for physical address extension (PAE) */
            if((flags & CPUID_FEATURE_PAE) && (ext_flags & CPUID_FEATURE_NXE)) {
                cpuinfo.features |= CPUINFO_FEATURE_PAE;

                /* max physical memory size */
                if(cpuid_ext_max >= 0x80000008) {
                    regs.eax = 0x80000008;
                    (void)cpuid(&regs);

                    cpuinfo.maxphyaddr = regs.eax & 0xff;
                }
            }
        }
    }
}

unsigned int machine_get_cpu_dcache_alignment(void) {
    return cpuinfo.dcache_alignment;
}
