/*
 * Copyright (C) 2024-2026 Philippe Aubertin.
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

#ifndef _JINUE_SHARED_ASM_MACHINE_H
#define _JINUE_SHARED_ASM_MACHINE_H

/* Do not assign zero here because this is what an undefined macro compares
 * equal to. Must match values in header.mk. */

/** AMD64 architecture - set in JINUE_CONFIG_ARCH */
#define JINUE_ARCH_AMD64    1

/** i686 architecture - set in JINUE_CONFIG_ARCH */
#define JINUE_ARCH_I686     2

/* If an architecture is specified through JINUE_CONFIG_ARCH, we configure the
 * header files for that architecture. Otherwise, we use the macros predefined
 * by the compiler.
 * 
 * The main use case for this is when compiling the setup code for the AMD64
 * kernel where, even though we are compiling 32-bit code, we need the AMD64
 * definitions. */

#if JINUE_CONFIG_ARCH == JINUE_ARCH_AMD64 || (!defined(JINUE_BUILD_ARCH) && defined(__x86_64__))
#define JINUE_ARCH_IS_AMD64
#endif

#if JINUE_CONFIG_ARCH == JINUE_ARCH_I686 || (!defined(JINUE_BUILD_ARCH) && defined(__i386__))
#define JINUE_ARCH_IS_I686
#endif

#ifdef JINUE_ARCH_IS_AMD64
#include <jinue/shared/asm/amd64.h>
#endif

#ifdef JINUE_ARCH_IS_I686
#include <jinue/shared/asm/i686.h>
#endif

#endif
