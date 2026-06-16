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

#ifndef JINUE_KERNEL_TYPEDEPS_H
#define JINUE_KERNEL_TYPEDEPS_H

/** This header file exists to resolve circular dependencies. <kernel/types.h>
 * includes a machine-dependent type definitions header file through
 * <kernel/machine/types.h>. This header file contain definitions needed by it
 * and isn't intended to be included elsewhere.
 * 
 *      <kernel/typedeps.h>
 *              ^
 *              |
 *      <kernel/infrastructure/amd64/exports/types.h> for amd64
 *              or
 *      <kernel/infrastructure/i686/exports/types.h> for i686
 *              ^
 *              |
 *      <kernel/machine/types.h>
 *              ^
 *              |
 *       <kernel/types.h>
 * */

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/** Virtual memory address (pointer) with pointer arithmetic allowed */
typedef unsigned char *addr_t;

#endif
