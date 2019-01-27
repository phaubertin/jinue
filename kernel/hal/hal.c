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
#include <hal/kernel.h>
#include <hal/pfaddr.h>
#include <hal/thread.h>
#include <hal/trap.h>
#include <hal/vga.h>
#include <hal/vm.h>
#include <hal/x86.h>
#include <panic.h>
#include <pfalloc.h>
#include <printk.h>
#include <stdbool.h>
#include <stdint.h>
#include <syscall.h>
#include <types.h>
#include <util.h>
#include <vm_alloc.h>


/** top of region of memory mapped 1:1 (kernel image plus some pages for
    data structures allocated during initialization) */
addr_t kernel_region_top;

/** current top of boot heap */
void *boot_heap;

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

static pseudo_descriptor_t *hal_allocate_pseudo_descriptor(void) {
    pseudo_descriptor_t *pseudo;

    boot_heap = ALIGN_END(boot_heap, sizeof(pseudo_descriptor_t));

    pseudo    = (pseudo_descriptor_t *)boot_heap;
    boot_heap = (pseudo_descriptor_t *)boot_heap + 1;

    return pseudo;
}

static void hal_init_segments(cpu_data_t *cpu_data) {
    /* Pseudo-descriptor allocation is temporary for the duration of this
     * function only. Remember heap pointer on entry so we can free the
     * pseudo-allocator when we are done. */
    addr_t boot_heap_on_entry = boot_heap;

    pseudo_descriptor_t *pseudo = hal_allocate_pseudo_descriptor();

    /* load new GDT and TSS */
    pseudo->addr    = (addr_t)&cpu_data->gdt;
    pseudo->limit   = GDT_LENGTH * 8 - 1;

    lgdt(pseudo);

    set_cs( SEG_SELECTOR(GDT_KERNEL_CODE, RPL_KERNEL) );
    set_ss( SEG_SELECTOR(GDT_KERNEL_DATA, RPL_KERNEL) );
    set_data_segments( SEG_SELECTOR(GDT_KERNEL_DATA, RPL_KERNEL) );
    set_gs( SEG_SELECTOR(GDT_PER_CPU_DATA, RPL_KERNEL) );

    ltr( SEG_SELECTOR(GDT_TSS, RPL_KERNEL) );

    /* Free pseudo-descriptor. */
    boot_heap = boot_heap_on_entry;
}

static void hal_init_idt(void) {
    unsigned int idx;

    addr_t boot_heap_on_entry = boot_heap;

    pseudo_descriptor_t *pseudo = hal_allocate_pseudo_descriptor();

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

    pseudo->addr  = (addr_t)idt;
    pseudo->limit = IDT_VECTOR_COUNT * sizeof(seg_descriptor_t) - 1;
    lidt(pseudo);

    /* Free pseudo-descriptor. */
    boot_heap = boot_heap_on_entry;
}

void hal_init(void) {
    cpu_data_t          *cpu_data;
    unsigned int         idx;
    pfaddr_t            *page_stack_buffer;
    
    /* pfalloc() should not be called yet -- use pfalloc_early() instead */
    use_pfalloc_early = true;
    
    (void)boot_info_check(true);
    
    const boot_info_t *boot_info = get_boot_info();

    /** ASSERTION: we assume the image starts on a page boundary */
    assert(page_offset_of(boot_info->image_start) == 0);

    /** ASSERTION: we assume the kernel starts on a page boundary */
    assert(page_offset_of(boot_info->kernel_start) == 0);

    printk("Kernel size is %u bytes.\n", boot_info->kernel_size);
    
    if(boot_info->ramdisk_start == 0 || boot_info->ramdisk_size == 0) {
        printk("%kWarning: no initial RAM disk.\n", VGA_COLOR_YELLOW);
    }
    else {
        printk("RAM disk with size %u bytes loaded at address %x.\n", boot_info->ramdisk_size, boot_info->ramdisk_start);
    }

    printk("Kernel command line:\n", boot_info->kernel_size);
    printk("    %s\n", boot_info->cmdline);

    /* This must be done before any boot heap allocation. */
    boot_heap = boot_info->boot_heap;

    /* This must be done before any call to pfalloc_early(). */
    kernel_region_top = boot_info->boot_end;
    
    /* get cpu info */
    cpu_detect_features();
    
    /* allocate per-CPU data
     * 
     * We need to ensure that the Task State Segment (TSS) contained in this
     * memory block does not cross a page boundary. */
    assert(sizeof(cpu_data_t) < CPU_DATA_ALIGNMENT);
    
    boot_heap = ALIGN_END(boot_heap, CPU_DATA_ALIGNMENT);
    
    cpu_data  = boot_heap;
    boot_heap = cpu_data + 1;
    
    /* initialize per-CPU data */
    cpu_init_data(cpu_data);
    
    /* initialize the page frame allocator */
    page_stack_buffer = (pfaddr_t *)pfalloc_early();
    init_pfcache(&global_pfcache, page_stack_buffer);

    for(idx = 0; idx < KERNEL_PAGE_STACK_INIT; ++idx) {
        pffree( EARLY_PTR_TO_PFADDR( pfalloc_early() ) );
    }

    /* initialize virtual memory management, enable paging
     *
     * below this point, it is no longer safe to call pfalloc_early() */
    bool use_pae = cpu_has_feature(CPU_FEATURE_PAE);
    vm_boot_init(boot_info, use_pae, cpu_data);
    
    /* Initialize GDT and TSS */
    hal_init_segments(cpu_data);

    /* initialize interrupt descriptor table (IDT) */
    hal_init_idt();

    /* Initialize virtual memory allocator and VM management caches. */
    vm_boot_postinit(boot_info, use_pae);
    
    /* choose system call method */
    hal_select_syscall_method();
}
