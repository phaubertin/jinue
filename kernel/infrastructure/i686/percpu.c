/*
 * Copyright (C) 2019-2024 Philippe Aubertin.
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

#include <kernel/infrastructure/i686/descriptors.h>
#include <kernel/infrastructure/i686/percpu.h>
#include <kernel/machine/tls.h>
#include <string.h>

static void set_tls_segment(percpu_t *data, void *addr, size_t size) {
    data->gdt[GDT_USER_TLS_DATA] = SEG_DESCRIPTOR(
        addr,
        size - 1,
        SEG_TYPE_DATA | SEG_FLAG_USER | SEG_FLAG_NORMAL
    );
}

void machine_set_thread_local_storage(const thread_t *thread) {
    percpu_t *data = get_percpu_data();
    set_tls_segment(data, thread->local_storage_addr, thread->local_storage_size);
}

void init_percpu_data(percpu_t *data) {
    tss_t *tss;
    
    tss = &data->tss;
    
    /* initialize with zeroes  */
    memset(data, '\0', sizeof(percpu_t));
    
    data->self                      = data;
    data->current_addr_space        = NULL;
    
    /* initialize GDT */
    data->gdt[GDT_NULL] = SEG_DESCRIPTOR(0, 0, 0);
    
    data->gdt[GDT_KERNEL_CODE] =
        SEG_DESCRIPTOR( 0,      0xfffff,                SEG_TYPE_CODE  | SEG_FLAG_KERNEL | SEG_FLAG_NORMAL);
    
    data->gdt[GDT_KERNEL_DATA] =
        SEG_DESCRIPTOR( 0,      0xfffff,                SEG_TYPE_DATA  | SEG_FLAG_KERNEL | SEG_FLAG_NORMAL);
    
    data->gdt[GDT_USER_CODE] =
        SEG_DESCRIPTOR( 0,      0xfffff,                SEG_TYPE_CODE  | SEG_FLAG_USER   | SEG_FLAG_NORMAL);
    
    data->gdt[GDT_USER_DATA] =
        SEG_DESCRIPTOR( 0,      0xfffff,                SEG_TYPE_DATA  | SEG_FLAG_USER   | SEG_FLAG_NORMAL);
    
    data->gdt[GDT_TSS] =
        SEG_DESCRIPTOR( tss,    TSS_LIMIT-1,            SEG_TYPE_TSS   | SEG_FLAG_KERNEL | SEG_FLAG_TSS);
    
    data->gdt[GDT_PER_CPU_DATA] =
        SEG_DESCRIPTOR( data,   sizeof(percpu_t)-1,   SEG_TYPE_DATA  | SEG_FLAG_KERNEL | SEG_FLAG_32BIT | SEG_FLAG_IN_BYTES | SEG_FLAG_NOSYSTEM | SEG_FLAG_PRESENT);
    
    set_tls_segment(data, NULL, 0);
    
    /* setup kernel stack in TSS */
    tss->ss0  = SEG_SELECTOR(GDT_KERNEL_DATA, RPL_KERNEL);
    tss->ss1  = SEG_SELECTOR(GDT_KERNEL_DATA, RPL_KERNEL);
    tss->ss2  = SEG_SELECTOR(GDT_KERNEL_DATA, RPL_KERNEL);

    /* kernel stack address is updated by machine_switch_thread() */
    tss->esp0 = NULL;
    tss->esp1 = NULL;
    tss->esp2 = NULL;
    
    /* From Intel 64 and IA-32 Architectures Software Developer's Manual Volume
     * 3 System Programming Guide chapter 16.5:
     * 
     * " If the I/O bit map base address is greater than or equal to the TSS
     *   segment limit, there is no I/O permission map, and all I/O instructions
     *   generate exceptions when the CPL is greater than the current IOPL. " */
    tss->iomap = TSS_LIMIT;
}
