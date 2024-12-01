/*
 * Copyright (C) 2023-2024 Philippe Aubertin.
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

#ifndef _JINUE_SHARED_ASM_MEMTYPE_H
#define _JINUE_SHARED_ASM_MEMTYPE_H

/* Assignments: use the same values as the ACPI/e820 address range types. Use
 * the 0xf0000000 to 0xffffffff OEM defined range for custom values. */

#define JINUE_MEMYPE_MEMORY             1

#define JINUE_MEMYPE_RESERVED           2

#define JINUE_MEMYPE_ACPI               3

#define JINUE_MEMYPE_NVS                4

#define JINUE_MEMYPE_UNUSABLE           5

#define JINUE_MEMYPE_DISABLED           6

#define JINUE_MEMYPE_PERSISTENT         7

#define JINUE_MEMYPE_OEM                12

#define JINUE_MEMYPE_KERNEL_RESERVED    0xf0000000

#define JINUE_MEMYPE_KERNEL_IMAGE       (JINUE_MEMYPE_KERNEL_RESERVED + 1)

#define JINUE_MEMYPE_RAMDISK            (JINUE_MEMYPE_KERNEL_RESERVED + 2)

#define JINUE_MEMYPE_LOADER_AVAILABLE   (JINUE_MEMYPE_KERNEL_RESERVED + 3)

#endif
