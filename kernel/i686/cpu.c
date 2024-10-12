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

#include <kernel/i686/cpu.h>
#include <kernel/i686/descriptors.h>
#include <kernel/i686/x86.h>
#include <kernel/machine/cpu.h>
#include <stdint.h>
#include <string.h>

cpu_info_t cpu_info;

void cpu_init_data(cpu_data_t *data) {
    tss_t *tss;
    
    tss = &data->tss;
    
    /* initialize with zeroes  */
    memset(data, '\0', sizeof(cpu_data_t));
    
    data->self                      = data;
    data->current_addr_space        = NULL;
    
    /* initialize GDT */
    data->gdt[GDT_NULL] = SEG_DESCRIPTOR(0, 0, 0);
    
    data->gdt[GDT_KERNEL_CODE] =
        SEG_DESCRIPTOR( 0,      0xfffff,                SEG_TYPE_CODE  | SEG_FLAG_KERNEL | SEG_FLAG_NORMAL);
    
    data->gdt[GDT_KERNEL_DATA] =
        SEG_DESCRIPTOR( 0,      0xfffff,                SEG_TYPE_DATA  | SEG_FLAG_KERNEL | SEG_FLAG_NORMAL);
    
    data->gdt[GDT_USER_CODE] =
        SEG_DESCRIPTOR( 0,      0xfffff,                SEG_TYPE_CODE  | SEG_FLAG_USER   | SEG_FLAG_NORMAL);
    
    data->gdt[GDT_USER_DATA] =
        SEG_DESCRIPTOR( 0,      0xfffff,                SEG_TYPE_DATA  | SEG_FLAG_USER   | SEG_FLAG_NORMAL);
    
    data->gdt[GDT_TSS] =
        SEG_DESCRIPTOR( tss,    TSS_LIMIT-1,            SEG_TYPE_TSS   | SEG_FLAG_KERNEL | SEG_FLAG_TSS);
    
    data->gdt[GDT_PER_CPU_DATA] =
        SEG_DESCRIPTOR( data,   sizeof(cpu_data_t)-1,   SEG_TYPE_DATA  | SEG_FLAG_KERNEL | SEG_FLAG_32BIT | SEG_FLAG_IN_BYTES | SEG_FLAG_NOSYSTEM | SEG_FLAG_PRESENT);
    
    data->gdt[GDT_USER_TLS_DATA] = SEG_DESCRIPTOR(0, 0, 0);
    
    /* setup kernel stack in TSS */
    tss->ss0  = SEG_SELECTOR(GDT_KERNEL_DATA, RPL_KERNEL);
    tss->ss1  = SEG_SELECTOR(GDT_KERNEL_DATA, RPL_KERNEL);
    tss->ss2  = SEG_SELECTOR(GDT_KERNEL_DATA, RPL_KERNEL);

    /* kernel stack address is updated by machine_switch_thread() */
    tss->esp0 = NULL;
    tss->esp1 = NULL;
    tss->esp2 = NULL;
    
    /* From Intel 64 and IA-32 Architectures Software Developer's Manual Volume
     * 3 System Programming Guide chapter 16.5:
     * 
     * " If the I/O bit map base address is greater than or equal to the TSS
     *   segment limit, there is no I/O permission map, and all I/O instructions
     *   generate exceptions when the CPL is greater than the current IOPL. " */
    tss->iomap = TSS_LIMIT;
}

void cpu_detect_features(void) {
    uint32_t temp_eflags;
    
    /* default values */
    cpu_info.maxphyaddr         = 32;
    cpu_info.dcache_alignment   = 32;
    cpu_info.features           = 0;
    cpu_info.vendor             = CPU_VENDOR_GENERIC;
    cpu_info.family             = 0;
    cpu_info.model              = 0;
    cpu_info.stepping           = 0;
    
    /* The CPUID instruction is available if we can change the value of eflags
     * bit 21 (ID) */
    temp_eflags  = get_eflags();
    temp_eflags ^= CPU_EFLAGS_ID;
    set_eflags(temp_eflags);
    
    if(temp_eflags == get_eflags()) {
        cpu_info.features |= CPU_FEATURE_CPUID;
    }
    
    if(cpu_has_feature(CPU_FEATURE_CPUID)) {
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
        if(    vendor_dw0 == CPU_VENDOR_AMD_DW0 
            && vendor_dw1 == CPU_VENDOR_AMD_DW1
            && vendor_dw2 == CPU_VENDOR_AMD_DW2) {
                
            cpu_info.vendor = CPU_VENDOR_AMD;
        }
        else if (vendor_dw0 == CPU_VENDOR_INTEL_DW0
            &&   vendor_dw1 == CPU_VENDOR_INTEL_DW1
            &&   vendor_dw2 == CPU_VENDOR_INTEL_DW2) {
                
            cpu_info.vendor = CPU_VENDOR_INTEL;
        }
        
        /* get processor signature (family/model/stepping) and feature flags */
        if(cpuid_max >= 1) {
            /* function 1: processor signature and feature flags */
            regs.eax = 1;

            /* call CPUID instruction */
            uint32_t signature = cpuid(&regs);

            /* set processor signature */
            cpu_info.stepping  = signature       & 0xf;
            cpu_info.model     = (signature>>4)  & 0xf;
            cpu_info.family    = (signature>>8)  & 0xf;

            /* feature flags */
            flags = regs.edx;

            /* cache alignment */
            if(flags & CPUID_FEATURE_CLFLUSH) {
                cpu_info.dcache_alignment = ((regs.ebx >> 8) & 0xff) * 8;
            }

            /* global pages */
            if(flags & CPUID_FEATURE_PGE) {
                cpu_info.features |= CPU_FEATURE_PGE;
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
            if(cpu_info.vendor == CPU_VENDOR_AMD) {
                cpu_info.features |= CPU_FEATURE_SYSENTER;
            }
            else if(cpu_info.vendor == CPU_VENDOR_INTEL) {
                if(cpu_info.family == 6 && cpu_info.model < 3 && cpu_info.stepping < 3) {
                    /* not supported */
                }
                else {
                    cpu_info.features |= CPU_FEATURE_SYSENTER;
                }
            }
        }

        /* support for SYSCALL/SYSRET instructions */
        if(cpu_info.vendor == CPU_VENDOR_AMD) {
            if(ext_flags & CPUID_EXT_FEATURE_SYSCALL) {
                cpu_info.features |= CPU_FEATURE_SYSCALL;
            }
        }

        if(cpu_info.vendor == CPU_VENDOR_AMD || cpu_info.vendor == CPU_VENDOR_INTEL) {
            /* support for local APIC */
            if(flags & CPUID_FEATURE_APIC) {
                cpu_info.features |= CPU_FEATURE_LOCAL_APIC;
            }

            /* large 4MB pages in 32-bit (non-PAE) paging mode */
            if(flags & CPUID_FEATURE_PSE) {
                cpu_info.features |= CPU_FEATURE_PSE;
            }

            /* support for physical address extension (PAE) */
            if((flags & CPUID_FEATURE_PAE) && (ext_flags & CPUID_FEATURE_NXE)) {
                cpu_info.features |= CPU_FEATURE_PAE;

                /* max physical memory size */
                if(cpuid_ext_max >= 0x80000008) {
                    regs.eax = 0x80000008;
                    (void)cpuid(&regs);

                    cpu_info.maxphyaddr = regs.eax & 0xff;
                }
            }
        }
    }
}

unsigned int machine_get_cpu_dcache_alignment(void) {
    return cpu_info.dcache_alignment;
}
