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

#include <kernel/infrastructure/i686/cpuidsig.h>

 /**
 * Map a CPUID signature to an ID for the kernel's internal use
 * 
 * For use with:
 *  - Vendor signature in CPUID leaf 0x00000000
 *  - Hypervisor signature in CPUID leaf 0x40000000
 * 
 * This function tries the entries in mapping one by one until one matches, so
 * it should be terminated by a default entry that matches everything, e.g.:
 * 
 *      static const cpuid_signature_t mapping[] = {
 *          // ...other entries...
 *          {
 *              .id             = DEFAULT_ID,
 *              .signature_ebx  = CPUID_SIGNATURE_ANY,
 *              .signature_ecx  = CPUID_SIGNATURE_ANY,
 *              .signature_edx  = CPUID_SIGNATURE_ANY
 *          }
 *      };
 * 
 * @param regs relevant CPUID leaf
 * @param mapping mapping entries
 */
int map_cpuid_signature(const x86_cpuid_regs_t *regs, const cpuid_signature_t mapping[]) {
    for(int idx = 0;; ++idx) {
        const cpuid_signature_t *entry = &mapping[idx];

        if(regs->ebx != entry->signature_ebx && entry->signature_ebx != CPUID_SIGNATURE_ANY) {
            continue;
        }

        if(regs->ecx != entry->signature_ecx && entry->signature_ecx != CPUID_SIGNATURE_ANY) {
            continue;
        }

        if(regs->edx != entry->signature_edx && entry->signature_edx != CPUID_SIGNATURE_ANY) {
            continue;
        }

        return mapping[idx].id;
    }
}
