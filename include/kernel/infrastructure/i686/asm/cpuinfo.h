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

#ifndef JINUE_KERNEL_INFRASTRUCTURE_I686_ASM_CPUINFO_H
#define JINUE_KERNEL_INFRASTRUCTURE_I686_ASM_CPUINFO_H

/* features */

#define CPUINFO_FEATURE_SYSENTER    (1<<0)

#define CPUINFO_FEATURE_SYSCALL     (1<<1)

#define CPUINFO_FEATURE_LOCAL_APIC  (1<<2)

#define CPUINFO_FEATURE_PAE         (1<<3)

#define CPUINFO_FEATURE_PGE         (1<<4)

#define CPUINFO_FEATURE_PSE         (1<<5)

/* vendors */

#define CPUINFO_VENDOR_GENERIC      0

#define CPUINFO_VENDOR_AMD          1

#define CPUINFO_VENDOR_INTEL        2

/* hypervisors */

#define HYPERVISOR_ID_NONE          0

#define HYPERVISOR_ID_UNKNOWN       1

#define HYPERVISOR_ID_ACRN          2

#define HYPERVISOR_ID_BHYVE         3

#define HYPERVISOR_ID_HYPER_V       4

#define HYPERVISOR_ID_KVM           5

#define HYPERVISOR_ID_QEMU          6

#define HYPERVISOR_ID_VMWARE        7

#define HYPERVISOR_ID_XEN           8

#endif
