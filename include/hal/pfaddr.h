/*
 * Copyright (C) 2017 Philippe Aubertin.
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

#ifndef JINUE_HAL_PFADDR_H
#define JINUE_HAL_PFADDR_H

#include <jinue-common/pfaddr.h>
#include <stdint.h>

/** convert an address in an integer to a page frame address */
#define ADDR_TO_PFADDR(x)   ( (pfaddr_t)(  (uint64_t)(x) >> PFADDR_SHIFT ) )

/** convert a page frame address to an address in an integer */
#define PFADDR_TO_ADDR(x)   ((uint64_t)(x) << PFADDR_SHIFT)

/** ensure page frame address is valid (LSBs zero) */
#define PFADDR_CHECK(x)     ( ( (uint32_t)(x) << (32 - PAGE_BITS + PFADDR_SHIFT) ) == 0 )

/** check if the page frame address is below the 4GB (32-bit) limit */
#define PFADDR_CHECK_4GB(x) ( ( (uint32_t)(x) >> (32 - PFADDR_SHIFT) ) == 0 )

#endif
