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

#ifndef JINUE_KERNEL_INFRASTRUCTURE_I686_ASM_CPUID_H
#define JINUE_KERNEL_INFRASTRUCTURE_I686_ASM_CPUID_H

/* Basic leaf 0 (0x00000000) ebx, ecx, edx */

#define CPUID_VENDOR_AMD_EBX            0x68747541  /* Auth */
#define CPUID_VENDOR_AMD_ECX            0x69746e65  /* enti */
#define CPUID_VENDOR_AMD_EDX            0x444d4163  /* cAMD */

#define CPUID_VENDOR_INTEL_EBX          0x756e6547  /* Genu */
#define CPUID_VENDOR_INTEL_ECX          0x49656e69  /* ineI */
#define CPUID_VENDOR_INTEL_EDX          0x6c65746e  /* ntel */

/* Basic leaf 1 (0x00000001) ecx */

#define CPUID_FEATURE_HYPERVISOR        (1<<31)

/* Basic leaf 1 (0x00000001) edx */

#define CPUID_FEATURE_FPU               (1<<0)

#define CPUID_FEATURE_PSE               (1<<3)

#define CPUID_FEATURE_PAE               (1<<6)

#define CPUID_FEATURE_APIC              (1<<9)

#define CPUID_FEATURE_SEP               (1<<11)

#define CPUID_FEATURE_PGE               (1<<13)

#define CPUID_FEATURE_CLFLUSH           (1<<19)

#define CPUID_FEATURE_HTT               (1<<28)

/* Software/hypervisor leaf 0 (0x40000000) ebx, ecx, edx */


#define CPUID_HYPERVISOR_ACRN           0x4e524341  /* ACRN */

#define CPUID_HYPERVISOR_BHYVE_EBX      0x76796862  /* bhyv */
#define CPUID_HYPERVISOR_BHYVE_ECX      0x68622065  /* e bh */
#define CPUID_HYPERVISOR_BHYVE_EDX      0x20657679  /* hyve */

#define CPUID_HYPERVISOR_KVM_EBX        0x4b4d564b  /* KVMK */
#define CPUID_HYPERVISOR_KVM_ECX        0x564b4d56  /* VMKV */
#define CPUID_HYPERVISOR_KVM_EDX        0x0000004d  /* M */

#define CPUID_HYPERVISOR_HYPER_V_EBX    0x7263694d  /* Micr */
#define CPUID_HYPERVISOR_HYPER_V_ECX    0x666f736f  /* osof */
#define CPUID_HYPERVISOR_HYPER_V_EDX    0x76482074  /* t Hv */

#define CPUID_HYPERVISOR_QEMU_EBX       0x54474354  /* TCGT */
#define CPUID_HYPERVISOR_QEMU_ECX       0x43544743  /* CGTC */
#define CPUID_HYPERVISOR_QEMU_EDX       0x47435447  /* GTCG */

#define CPUID_HYPERVISOR_VMWARE_EBX     0x61774d56  /* VMwa */
#define CPUID_HYPERVISOR_VMWARE_ECX     0x4d566572  /* reVM */
#define CPUID_HYPERVISOR_VMWARE_EDX     0x65726177  /* ware */

#define CPUID_HYPERVISOR_XEN_EBX        0x566e6558  /* XenV */
#define CPUID_HYPERVISOR_XEN_ECX        0x65584d4d  /* MMXe */
#define CPUID_HYPERVISOR_XEN_EDX        0x4d4d566e  /* nVMM */

/* Extended leaf 1 (0x8000001) edx */

#define CPUID_EXT_FEATURE_SYSCALL       (1<<11)

#define CPUID_FEATURE_NXE               (1<<20)

#endif
