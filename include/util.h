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

#ifndef JINUE_KERNEL_UTIL_H
#define JINUE_KERNEL_UTIL_H

#include <stddef.h>
#include <types.h>

#define ALIGN_START(x, s)       ( (x) & ~((s)-1) )

#define ALIGN_END(x, s)         ( ALIGN_START((x) + s - 1, (s)) )

#define OFFSET_OF_PTR(x, s)     ( (uintptr_t)(x) & ((s)-1) )

#define ALIGN_START_PTR(x, s)   ( (void *)ALIGN_START((uintptr_t)(x), (s)) )

#define ALIGN_END_PTR(x, s)     ( (void *)ALIGN_END((uintptr_t)(x), (s)) )


static inline void *alloc_forward_func(size_t size, void **alloc_ptr) {
	char *ret = *alloc_ptr;

	*alloc_ptr = ret + size;

	return ret;
}

static inline void *alloc_backward_func(size_t size, void **alloc_ptr) {
	char *ret = *alloc_ptr;

	*alloc_ptr = ret - size;

	return *alloc_ptr;
}

#define alloc_forward(T, p) ((T *)alloc_forward_func(sizeof(T), &(p)))

#define alloc_backward(T, p) ((T *)alloc_forward_func(sizeof(T), &(p)))

#endif
