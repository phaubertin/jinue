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

#ifndef JINUE_KERNEL_INFRASTRUCTURE_I686_ASM_EFLAGS_H
#define JINUE_KERNEL_INFRASTRUCTURE_I686_ASM_EFLAGS_H

#define EFLAGS_CF       (1<<0)

#define EFLAGS_ALWAYS_1 (1<<1)

#define EFLAGS_PF       (1<<2)

#define EFLAGS_AF       (1<<4)

#define EFLAGS_ZF       (1<<6)

#define EFLAGS_SF       (1<<7)

#define EFLAGS_TF       (1<<8)

#define EFLAGS_IF       (1<<9)

#define EFLAGS_DF       (1<<10)

#define EFLAGS_OF       (1<<11)

#define EFLAGS_NT       (1<<14)

#define EFLAGS_RF       (1<<16)

#define EFLAGS_VM86     (1<<17)

#define EFLAGS_AC       (1<<18)

#define EFLAGS_VIF      (1<<19)

#define EFLAGS_VIP      (1<<20)

#define EFLAGS_ID       (1<<21)

#endif
