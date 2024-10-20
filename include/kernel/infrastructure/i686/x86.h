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

#ifndef JINUE_KERNEL_INFRASTRUCTURE_I686_X86_H
#define JINUE_KERNEL_INFRASTRUCTURE_I686_X86_H

#include <kernel/infrastructure/i686/asm/x86.h>
#include <kernel/infrastructure/i686/types.h>

typedef struct {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
} x86_cpuid_regs_t;

typedef uint32_t msr_addr_t;


void cli(void);

void sti(void);

void hlt(void);

void invalidate_tlb(void *vaddr);

void lgdt(pseudo_descriptor_t *gdt_info);

void lidt(pseudo_descriptor_t *idt_info);

void ltr(seg_selector_t sel);

uint32_t cpuid(x86_cpuid_regs_t *regs);

uint32_t get_esp(void);

uint32_t get_cr0(void);

uint32_t get_cr2(void);

uint32_t get_cr3(void);

uint32_t get_cr4(void);

void set_cr0(uint32_t val);

void set_cr3(uint32_t val);

void set_cr4(uint32_t val);

uint32_t get_eflags(void);

void set_eflags(uint32_t val);

void set_cs(uint32_t val);

void set_ds(uint32_t val);

void set_es(uint32_t val);

void set_fs(uint32_t val);

void set_gs(uint32_t val);

void set_ss(uint32_t val);

uint64_t rdmsr(msr_addr_t addr);

void wrmsr(msr_addr_t addr, uint64_t val);

uint32_t get_gs_ptr(uint32_t *ptr);

uint64_t rdtsc(void);

void x86_enable_pae(uint32_t cr3_value);

#endif

