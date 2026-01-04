/*
 * Copyright (C) 2026 Philippe Aubertin.
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

#ifndef JINUE_KERNEL_INFRASTRUCTURE_I686_ASM_CYRIX_H
#define JINUE_KERNEL_INFRASTRUCTURE_I686_ASM_CYRIX_H

/* See Cyrix 6x86 processor databoook section 2.4.4 and table 2-10 "6x86 CPU
 * Configuration Registers" */

#define CYRIX_CONF_INDEX_IOPORT 0x22

#define CYRIX_CONF_DATA_IOPORT  0x23


#define CYRIX_CONFREG_CCR0  0xc0

#define CYRIX_CONFREG_CCR1  0xc1

#define CYRIX_CONFREG_CCR2  0xc2

#define CYRIX_CONFREG_CCR3  0xc3

#define CYRIX_CONFREG_CCR4  0xe8

#define CYRIX_CONFREG_CCR5  0xe9


#define CYRIX_CONFREG_ARR0  0xc4

#define CYRIX_CONFREG_ARR1  0xc7

#define CYRIX_CONFREG_ARR2  0xca

#define CYRIX_CONFREG_ARR3  0xcd

#define CYRIX_CONFREG_ARR4  0xd0

#define CYRIX_CONFREG_ARR5  0xd3

#define CYRIX_CONFREG_ARR6  0xd6

#define CYRIX_CONFREG_ARR7  0xd9


#define CYRIX_CONFREG_RCR0  0xdc

#define CYRIX_CONFREG_RCR1  0xdd

#define CYRIX_CONFREG_RCR2  0xde

#define CYRIX_CONFREG_RCR3  0xdf

#define CYRIX_CONFREG_RCR4  0xe0

#define CYRIX_CONFREG_RCR5  0xe1

#define CYRIX_CONFREG_RCR6  0xe2

#define CYRIX_CONFREG_RCR7  0xe3


#define CYRIX_CONFREG_DIR0  0xfe

#define CYRIX_CONFREG_DIR1  0xff


#endif
