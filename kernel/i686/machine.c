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

#include <kernel/i686/boot.h>
#include <kernel/i686/cpu.h>
#include <kernel/i686/cpu_data.h>
#include <kernel/i686/descriptors.h>
#include <kernel/i686/interrupt.h>
#include <kernel/i686/machine.h>
#include <kernel/i686/memory.h>
#include <kernel/i686/pic8259.h>
#include <kernel/i686/remap.h>
#include <kernel/i686/thread.h>
#include <kernel/i686/trap.h>
#include <kernel/i686/vga.h>
#include <kernel/i686/vm.h>
#include <kernel/i686/vm_pae.h>
#include <kernel/i686/x86.h>
#include <kernel/boot.h>
#include <kernel/cmdline.h>
#include <kernel/logging.h>
#include <kernel/panic.h>
#include <kernel/page_alloc.h>
#include <kernel/syscall.h>
#include <kernel/util.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>


/** Specifies the entry point to use for system calls */
int syscall_implementation;

static void check_data_segment(const boot_info_t *boot_info) {
    if(     boot_info->data_start == 0 ||
            boot_info->data_size == 0 ||
            boot_info->data_physaddr == 0) {
        panic("Setup code wasn't able to load kernel data segment");
    }
}

static void check_alignment(const boot_info_t *boot_info) {
    if(page_offset_of(boot_info->image_start) != 0) {
        panic("Kernel image start is not aligned on a page boundary");
    }

    if(page_offset_of(boot_info->image_top) != 0) {
        panic("Top of kernel image is not aligned on a page boundary");
    }
}

static void move_kernel_at_16mb(const boot_info_t *boot_info) {
    move_and_remap_kernel(
            (addr_t)boot_info->page_table_1mb,
            (addr_t)boot_info->page_table_klimit,
            (uint32_t)boot_info->page_directory);

    vm_write_protect_kernel_image(boot_info);
}

static bool maybe_enable_pae(
        boot_alloc_t            *boot_alloc,
        const boot_info_t       *boot_info,
        const cmdline_opts_t    *cmdline_opts) {

    bool use_pae;

    if(cpu_has_feature(CPU_FEATURE_PAE)) {
        use_pae = (cmdline_opts->pae != CMDLINE_OPT_PAE_DISABLE);
    }
    else {
        if(cmdline_opts->pae == CMDLINE_OPT_PAE_REQUIRE) {
            panic("Option pae=require passed on kernel command line but PAE is not supported.");
        }

        use_pae = false;
    }

    if(! use_pae) {
        warning("Warning: Physical Address Extension (PAE) not enabled. NX protection disabled.");
        vm_set_no_pae();
    }
    else {
        info("Enabling Physical Address Extension (PAE).");
        vm_pae_enable(boot_alloc, boot_info);
    }

    return use_pae;
}

static void init_idt(void) {
    /* initialize IDT */
    for(int idx = 0; idx < IDT_VECTOR_COUNT; ++idx) {
        /* get address, which is already stored in the IDT entry */
        addr_t addr = (addr_t)(uintptr_t)idt[idx];

        /* set interrupt gate flags */
        unsigned int flags = SEG_TYPE_INTERRUPT_GATE | SEG_FLAG_NORMAL_GATE;

        if(idx == JINUE_SYSCALL_IRQ) {
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

static void remap_text_video_memory(boot_alloc_t *boot_alloc) {
    size_t video_buffer_size    = VGA_TEXT_VID_TOP - VGA_TEXT_VID_BASE;
    size_t num_pages            = video_buffer_size / PAGE_SIZE;

    void *buffer = boot_page_alloc_n(boot_alloc, num_pages);
    void *mapped = (void *)PHYS_TO_VIRT_AT_16MB(buffer);

    vm_boot_map(mapped, VGA_TEXT_VID_BASE, num_pages);

    info("Remapping text video memory at %#p", mapped);

    /* Note: after this call to vga_set_base_addr() until we switch to the new
     * new address space, VGA output is not possible. Attempting it will cause
     * a kernel panic due to a page fault (and the panic handler itself attempts
     * to log). */
    vga_set_base_addr(mapped);
}

static void enable_global_pages(void) {
    if(cpu_has_feature(CPU_FEATURE_PGE)) {
        set_cr4(get_cr4() | X86_CR4_PGE);
    }
}

static void initialize_page_allocator(boot_alloc_t *boot_alloc) {
    while(! boot_page_alloc_is_empty(boot_alloc)) {
        page_free(boot_page_alloc(boot_alloc));
    }

    info(
            "%u kilobytes available for allocation by the kernel",
            get_page_count() * PAGE_SIZE / (1 * KB));
}

static void init_descriptors(cpu_data_t *cpu_data, boot_alloc_t *boot_alloc) {
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

    /* free pseudo-descriptor. */
    boot_heap_pop(boot_alloc);
}

static void select_syscall_implementation(void) {
    if(cpu_has_feature(CPU_FEATURE_SYSCALL)) {
        uint64_t msrval;

        syscall_implementation = JINUE_SYSCALL_IMPL_FAST_AMD;

        msrval  = rdmsr(MSR_EFER);
        msrval |= MSR_FLAG_EFER_SCE;
        wrmsr(MSR_EFER, msrval);

        msrval  = (uint64_t)(uintptr_t)fast_amd_entry;
        msrval |= (uint64_t)SEG_SELECTOR(GDT_KERNEL_CODE, RPL_KERNEL)   << 32;
        msrval |= (uint64_t)SEG_SELECTOR(GDT_USER_CODE,   RPL_USER)     << 48;

        wrmsr(MSR_STAR, msrval);
    }
    else if(cpu_has_feature(CPU_FEATURE_SYSENTER)) {
        syscall_implementation = JINUE_SYSCALL_IMPL_FAST_INTEL;

        wrmsr(MSR_IA32_SYSENTER_CS,  SEG_SELECTOR(GDT_KERNEL_CODE, RPL_KERNEL));
        wrmsr(MSR_IA32_SYSENTER_EIP, (uint64_t)(uintptr_t)fast_intel_entry);

        /* kernel stack address is set when switching thread context */
        wrmsr(MSR_IA32_SYSENTER_ESP, (uint64_t)(uintptr_t)NULL);
    }
    else {
        syscall_implementation = JINUE_SYSCALL_IMPL_INTERRUPT;
    }
}

void machine_init(
        Elf32_Ehdr              *kernel_elf,
        const cmdline_opts_t    *cmdline_opts,
        boot_alloc_t            *boot_alloc,
        const boot_info_t       *boot_info) {

    cpu_detect_features();

    check_data_segment(boot_info);

    check_alignment(boot_info);

    check_memory(boot_info);

    move_kernel_at_16mb(boot_info);

    bool pae_enabled = maybe_enable_pae(
            boot_alloc,
            boot_info,
            cmdline_opts);

    /* Re-initialize the boot page allocator to allocate following the kernel
     * image at 16MB rather than at 1MB, now that the kernel has been moved
     * there.
     *
     * Do this after enabling PAE: we want the temporary PAE page tables to be
     * allocated after 1MB because we won't need them anymore once the final
     * address space is created. */
    boot_reinit_at_16mb(boot_alloc);

    /* allocate per-CPU data
     * 
     * We need to ensure that the Task State Segment (TSS) contained in this
     * memory block does not cross a page boundary. */
    assert(sizeof(cpu_data_t) < CPU_DATA_ALIGNMENT);
    cpu_data_t *cpu_data = boot_heap_alloc(
            boot_alloc,
            cpu_data_t, CPU_DATA_ALIGNMENT);
    
    /* initialize per-CPU data */
    cpu_init_data(cpu_data);
    
    /* Initialize interrupt descriptor table (IDT)
     *
     * This function modifies the IDT in-place (see trap.asm). This must be
     * done before vm_boot_init() because the page protection bits set up by
     * vm_boot_init() prevent this. */
    init_idt();

    /* Initialize programmable interrupt_controller. */
    pic8259_init();

    addr_space_t *addr_space = vm_create_initial_addr_space(
            kernel_elf,
            boot_alloc,
            boot_info);

    memory_initialize_array(boot_alloc, boot_info);

    /* After this, VGA output is not possible until we switch to the
     * new address space (see the call to vm_switch_addr_space() below).
     * Attempting it will cause a kernel panic due to a page fault (and the
     * panic handler itself attempts to log). */
    remap_text_video_memory(boot_alloc);

    /* switch to new address space */
    vm_switch_addr_space(addr_space, cpu_data);

    enable_global_pages();

    /* From this point, we are ready to switch to the new address space, so we
     * don't need to allocate any more pages from the boot allocator. Transfer
     * the remaining pages to the run-time page allocator. */
    boot_reinit_at_klimit(boot_alloc);
    initialize_page_allocator(boot_alloc);

    /* Initialize GDT and TSS */
    init_descriptors(cpu_data, boot_alloc);

    /* create slab cache to allocate PDPTs
     *
     * This must be done after the global page allocator has been initialized
     * because the slab allocator needs to allocate a slab to allocate the new
     * slab cache on the slab cache cache.
     *
     * This must be done before the first time vm_create_addr_space() is called. */
    if(pae_enabled) {
        vm_pae_create_pdpt_cache();
    }

    /* choose a system call implementation */
    select_syscall_implementation();
}
