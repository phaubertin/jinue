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

#ifndef _JINUE_LIBC_STDINT_H
#define _JINUE_LIBC_STDINT_H

typedef signed char         int8_t;

typedef short               int16_t;

typedef int                 int32_t;

typedef long long           int64_t;


typedef unsigned char       uint8_t;

typedef unsigned short      uint16_t;

typedef unsigned int        uint32_t;

typedef unsigned long long  uint64_t;


typedef long                intptr_t;

typedef unsigned long       uintptr_t;


typedef long long           intmax_t;

typedef unsigned long long  uintmax_t;

#define INT8_C(x)           x

#define UINT8_C(x)          x

#define INT16_C(x)          x

#define UINT16_C(x)         x

#define INT32_C(x)          x

#define UINT32_C(x)         x

#define INT64_C(x)          x ## LL

#define UINT64_C(x)         x ## ULL

#define INTMAX_C(x)         x ## LL

#define UINTMAX_C(x)        x ## ULL

#endif
