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

#ifndef _JINUE_SHARED_ASM_AUXV_H
#define _JINUE_SHARED_ASM_AUXV_H

/** Last entry  */
#define JINUE_AT_NULL           0

/** Ignore entry */
#define JINUE_AT_IGNORE         1

/** Program file descriptor */
#define JINUE_AT_EXECFD         2

/** Program headers address */
#define JINUE_AT_PHDR           3

/** Size of program header entry */
#define JINUE_AT_PHENT          4

/** Number of program header entries */
#define JINUE_AT_PHNUM          5

/** Page size */
#define JINUE_AT_PAGESZ         6

/** Base address */
#define JINUE_AT_BASE           7

/** Flags */
#define JINUE_AT_FLAGS          8

/** Program entry point */
#define JINUE_AT_ENTRY          9

/** Stack base address for main thread */
#define JINUE_AT_STACKBASE      10

/** System call mechanism */
#define JINUE_AT_HOWSYSCALL     11

#endif
