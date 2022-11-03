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

#ifndef JINUE_HAL_DESCRIPTORS_H
#define JINUE_HAL_DESCRIPTORS_H

#include <kernel/hal/asm/descriptors.h>
#include <kernel/hal/types.h>


#define PACK_DESCRIPTOR(val, mask, shamt1, shamt2) \
    ( (((uint64_t)(uintptr_t)(val) >> shamt1) & mask) << shamt2 )

#define SEG_DESCRIPTOR(base, limit, type) (\
      PACK_DESCRIPTOR((type),  0xf0ff,  0, SEG_FLAGS_OFFSET) \
    | PACK_DESCRIPTOR((base),  0xff,   24, 56) \
    | PACK_DESCRIPTOR((base),  0xff,   16, 32) \
    | PACK_DESCRIPTOR((base),  0xffff,  0, 16) \
    | PACK_DESCRIPTOR((limit), 0xf,    16, 48) \
    | PACK_DESCRIPTOR((limit), 0xffff,  0,  0) \
)

#define GATE_DESCRIPTOR(segment, offset, type, param_count) (\
      PACK_DESCRIPTOR((type),        0xff,    0,  SEG_FLAGS_OFFSET) \
    | PACK_DESCRIPTOR((param_count), 0xf,     0,  32) \
    | PACK_DESCRIPTOR((segment),     0xffff,  0,  16) \
    | PACK_DESCRIPTOR((offset),      0xffff, 16,  48) \
    | PACK_DESCRIPTOR((offset),      0xffff,  0,   0) \
)

#endif
