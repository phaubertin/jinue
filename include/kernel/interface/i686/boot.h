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

#ifndef JINUE_KERNEL_INTERFACE_I686_BOOT_H
#define JINUE_KERNEL_INTERFACE_I686_BOOT_H

#include <kernel/infrastructure/i686/types.h>
#include <kernel/interface/i686/asm/boot.h>
#include <kernel/interface/i686/asm/bootinfo.h>
#include <kernel/interface/i686/types.h>
#include <stdbool.h>

bool check_bootinfo(bool panic_on_failure);

const bootinfo_t *get_bootinfo(void);

/**
 * Determine if specified feature was detected by the setup code
 * 
 * Use the BOOTINFO_FEATURE_... constants for the mask arguments. A bitwise or
 * of multiple of these constants is allowed, in which case this function will
 * return true only if all the specified features are supported.
 * 
 * The boot information (aka. bootinfo) structure is used to coommunicate
 * information between the setup code and the kernel proper. In general, in
 * order to determine if a CPU feature is supported, you should use the
 * cpu_has_feature() function instead.
 *
 * @param bootinfo the boot information structure
 * @param mask feature(s) for which to check support
 * @return true if feature is supported, false otherwise
 */
static inline bool bootinfo_has_feature(const bootinfo_t *bootinfo, uint32_t mask) {
    return (bootinfo->features & mask) == mask;
}

#endif
