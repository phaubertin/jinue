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

#include <assert.h>
#include <hal/boot.h>
#include <hal/cpu.h>
#include <hal/cpu_data.h>
#include <hal/descriptors.h>
#include <hal/hal.h>
#include <hal/interrupt.h>
#include <hal/mem.h>
#include <hal/pic8259.h>
#include <hal/thread.h>
#include <hal/trap.h>
#include <hal/vga.h>
#include <hal/vm.h>
#include <hal/x86.h>
#include <boot.h>
#include <panic.h>
#include <pfalloc.h>
#include <printk.h>
#include <stdbool.h>
#include <stdint.h>
#include <syscall.h>
#include <util.h>


/** Specifies the entry point to use for system calls */
int syscall_method;

static void hal_select_syscall_method(void) {
    if(cpu_has_feature(CPU_FEATURE_SYSCALL)) {
        uint64_t msrval;

        syscall_method = SYSCALL_METHOD_FAST_AMD;

        msrval  = rdmsr(MSR_EFER);
        msrval |= MSR_FLAG_STAR_SCE;
        wrmsr(MSR_EFER, msrval);

        msrval  = (uint64_t)(uintptr_t)fast_amd_entry;
        msrval |= (uint64_t)SEG_SELECTOR(GDT_KERNEL_CODE, RPL_KERNEL)   << 32;
        msrval |= (uint64_t)SEG_SELECTOR(GDT_USER_CODE,   RPL_USER)     << 48;

        wrmsr(MSR_STAR, msrval);
    }
    else if(cpu_has_feature(CPU_FEATURE_SYSENTER)) {
        syscall_method = SYSCALL_METHOD_FAST_INTEL;

        wrmsr(MSR_IA32_SYSENTER_CS,  SEG_SELECTOR(GDT_KERNEL_CODE, RPL_KERNEL));
        wrmsr(MSR_IA32_SYSENTER_EIP, (uint64_t)(uintptr_t)fast_intel_entry);

        /* kernel stack address is set when switching thread context */
        wrmsr(MSR_IA32_SYSENTER_ESP, (uint64_t)(uintptr_t)NULL);
    }
    else {
        syscall_method = SYSCALL_METHOD_INTR;
    }
}

static void hal_init_descriptors(cpu_data_t *cpu_data, boot_alloc_t *boot_alloc) {
    /* Pseudo-descriptor allocation is temporary for the duration of this
     * function only. Remember heap pointer on entry so we can free the
     * pseudo-allocator when we are done. */
    boot_heap_push(boot_alloc);

    pseudo_descriptor_t *pseudo =
            boot_heap_alloc(boot_alloc, pseudo_descriptor_t, sizeof(pseudo_descriptor_t));

    /* load interrupt descriptor table */
    pseudo->addr  = (addr_t)idt;
    pseudo->limit = IDT_VECTOR_COUNT * sizeof(seg_descriptor_t) - 1;
    lidt(pseudo);

    /* load new GDT and TSS */
    pseudo->addr    = (addr_t)&cpu_data->gdt;
    pseudo->limit   = GDT_LENGTH * 8 - 1;

    lgdt(pseudo);

    /* load new segment descriptors */
    uint32_t code_selector      = SEG_SELECTOR(GDT_KERNEL_CODE,  RPL_KERNEL);
    uint32_t data_selector      = SEG_SELECTOR(GDT_KERNEL_DATA,  RPL_KERNEL);
    uint32_t per_cpu_selector   = SEG_SELECTOR(GDT_PER_CPU_DATA, RPL_KERNEL);

    set_cs(code_selector);
    set_ss(data_selector);
    set_ds(data_selector);
    set_es(data_selector);
    set_fs(data_selector);
    set_gs(per_cpu_selector);

    /* load TSS segment into task register */
    ltr( SEG_SELECTOR(GDT_TSS, RPL_KERNEL) );

    /* Free pseudo-descriptor. */
    boot_heap_pop(boot_alloc);
}

static void hal_init_idt(void) {
    unsigned int idx;

    /* initialize IDT */
    for(idx = 0; idx < IDT_VECTOR_COUNT; ++idx) {
        /* get address, which is already stored in the IDT entry */
        addr_t addr = (addr_t)(uintptr_t)idt[idx];

        /* set interrupt gate flags */
        unsigned int flags = SEG_TYPE_INTERRUPT_GATE | SEG_FLAG_NORMAL_GATE;

        if(idx == SYSCALL_IRQ) {
            flags |= SEG_FLAG_USER;
        }
        else {
            flags |= SEG_FLAG_KERNEL;
        }

        /* create interrupt gate descriptor */
        idt[idx] = GATE_DESCRIPTOR(
            SEG_SELECTOR(GDT_KERNEL_CODE, RPL_KERNEL),
            addr,
            flags,
            NULL );
    }
}

void hal_init(boot_alloc_t *boot_alloc, const boot_info_t *boot_info) {
    int idx;

    /* get cpu info */
    cpu_detect_features();
    
    /* allocate per-CPU data
     * 
     * We need to ensure that the Task State Segment (TSS) contained in this
     * memory block does not cross a page boundary. */
    assert(sizeof(cpu_data_t) < CPU_DATA_ALIGNMENT);
    
    cpu_data_t *cpu_data = boot_heap_alloc(boot_alloc, cpu_data_t, CPU_DATA_ALIGNMENT);
    
    /* initialize per-CPU data */
    cpu_init_data(cpu_data);
    
    /* Initialize interrupt descriptor table (IDT)
     *
     * This function modifies the IDT in-place (see trap.asm). This must be
     * done before vm_boot_init() because the page protection bits set up by
     * vm_boot_init() prevent this. */
    hal_init_idt();

    /* Initialize programmable interrupt_controller. */
    pic8259_init(IDT_PIC8259_BASE);

    /* initialize the page frame allocator */
    kern_paddr_t *page_stack_buffer = (kern_paddr_t *)boot_pgalloc_early(boot_alloc);
    init_pfalloc_cache(&global_pfalloc_cache, page_stack_buffer);

    for(idx = 0; idx < KERNEL_PAGE_STACK_INIT; ++idx) {
        pffree( EARLY_PTR_TO_PHYS_ADDR( boot_pgalloc_early(boot_alloc) ) );
    }

    /* initialize virtual memory management, enable paging
     *
     * below this point, it is no longer safe to call pfalloc_early() */
    bool use_pae = cpu_has_feature(CPU_FEATURE_PAE);
    vm_boot_init(boot_info, use_pae, cpu_data, boot_alloc);
    
    /* Initialize GDT and TSS */
    hal_init_descriptors(cpu_data, boot_alloc);

    /* Initialize virtual memory allocator and VM management caches. */
    vm_boot_postinit(boot_info, use_pae);
    
    /* choose system call method */
    hal_select_syscall_method();
}
