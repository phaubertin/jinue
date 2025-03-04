/*
 * Copyright (C) 2019-2025 Philippe Aubertin.
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

#include <jinue/shared/asm/mman.h>
#include <jinue/shared/asm/syscalls.h>
#include <kernel/domain/alloc/page_alloc.h>
#include <kernel/domain/services/cmdline.h>
#include <kernel/domain/services/logging.h>
#include <kernel/domain/services/mman.h>
#include <kernel/domain/services/panic.h>
#include <kernel/infrastructure/i686/asm/msr.h>
#include <kernel/infrastructure/i686/drivers/pic8259.h>
#include <kernel/infrastructure/i686/drivers/pit8253.h>
#include <kernel/infrastructure/i686/drivers/uart16550a.h>
#include <kernel/infrastructure/i686/drivers/vga.h>
#include <kernel/infrastructure/i686/firmware/acpi.h>
#include <kernel/infrastructure/i686/firmware/mp.h>
#include <kernel/infrastructure/i686/isa/instrs.h>
#include <kernel/infrastructure/i686/isa/regs.h>
#include <kernel/infrastructure/i686/pmap/init.h>
#include <kernel/infrastructure/i686/pmap/pae.h>
#include <kernel/infrastructure/i686/pmap/pmap.h>
#include <kernel/infrastructure/i686/boot_alloc.h>
#include <kernel/infrastructure/i686/cpuinfo.h>
#include <kernel/infrastructure/i686/descriptors.h>
#include <kernel/infrastructure/i686/memory.h>
#include <kernel/infrastructure/i686/percpu.h>
#include <kernel/infrastructure/elf.h>
#include <kernel/interface/i686/asm/idt.h>
#include <kernel/interface/i686/asm/irq.h>
#include <kernel/interface/i686/boot.h>
#include <kernel/interface/i686/interrupts.h>
#include <kernel/interface/i686/trap.h>
#include <kernel/interface/syscalls.h>
#include <kernel/machine/asm/machine.h>
#include <kernel/machine/init.h>
#include <kernel/utils/utils.h>
#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>


/** Specifies the entry point to use for system calls */
int syscall_implementation;

static void move_kernel_at_16mb(const bootinfo_t *bootinfo) {
    move_and_remap_kernel(
            (addr_t)bootinfo->page_table_1mb,
            (addr_t)bootinfo->page_table_klimit,
            (uint32_t)bootinfo->page_directory);

    pmap_write_protect_kernel_image(bootinfo);
}

static bool maybe_enable_pae(
        boot_alloc_t        *boot_alloc,
        const bootinfo_t   *bootinfo,
        const config_t      *config) {

    bool use_pae;

    if(cpu_has_feature(CPUINFO_FEATURE_PAE)) {
        use_pae = (config->machine.pae != CONFIG_PAE_DISABLE);
    }
    else {
        if(config->machine.pae == CONFIG_PAE_REQUIRE) {
            panic("Option pae=require passed on kernel command line but PAE is not supported.");
        }

        use_pae = false;
    }

    if(! use_pae) {
        warning("warning: physical Address Extension (PAE) not enabled. NX protection disabled.");
        pmap_set_no_pae();
    }
    else {
        info("Enabling Physical Address Extension (PAE).");
        pae_enable(boot_alloc, bootinfo);
    }

    return use_pae;
}

static void init_idt(void) {
    /* initialize IDT */
    for(int idx = 0; idx < IDT_VECTOR_COUNT; ++idx) {
        /* get address, which is already stored in the IDT entry */
        addr_t addr = (addr_t)(uintptr_t)idt[idx];

        /* Set interrupt gate flags.
         *
         * Because we are using an interrupt gate, the IF flag is cleared when
         * the interrupt routine is entered, which means interrupts are
         * disabled.
         *
         * See Intel 64 and IA-32 Architectures Software Developer’s Manual
         * Volume 3 section 7.12.1.3 "Flag Usage By Exception- or Interrupt-
         * Handler Procedure".
         */
        unsigned int flags = SEG_TYPE_INTERRUPT_GATE | SEG_FLAG_NORMAL_GATE;

        if(idx == JINUE_I686_SYSCALL_INTERRUPT) {
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

static void load_selectors(percpu_t *cpu_data) {
    pseudo_descriptor_t pseudo;

    /* load interrupt descriptor table */
    pseudo.addr  = (addr_t)idt;
    pseudo.limit = IDT_VECTOR_COUNT * sizeof(seg_descriptor_t) - 1;

    lidt(&pseudo);

    /* load new GDT */
    pseudo.addr    = (addr_t)&cpu_data->gdt;
    pseudo.limit   = GDT_NUM_ENTRIES * 8 - 1;

    lgdt(&pseudo);

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
}

static void enable_global_pages(void) {
    if(cpu_has_feature(CPUINFO_FEATURE_PGE)) {
        set_cr4(get_cr4() | X86_CR4_PGE);
    }
}

static void remap_text_video_memory(void) {
    void *mapped = map_in_kernel(
        VGA_TEXT_VID_BASE,
        VGA_TEXT_VID_TOP - VGA_TEXT_VID_BASE,
        JINUE_PROT_READ | JINUE_PROT_WRITE
    );

    vga_set_base_addr(mapped);

    info("Remapped text video memory at %#p", mapped);
}

static void initialize_page_allocator(boot_alloc_t *boot_alloc) {
    while(! boot_page_alloc_is_empty(boot_alloc)) {
        page_free(boot_page_alloc(boot_alloc));
    }

    info(   "%u kilobytes available for allocation by the kernel",
            get_page_count() * PAGE_SIZE / (1 * KB));
}

static void select_syscall_implementation(void) {
    if(cpu_has_feature(CPUINFO_FEATURE_SYSCALL)) {
        uint64_t msrval;

        syscall_implementation = JINUE_I686_HOWSYSCALL_FAST_AMD;

        msrval  = rdmsr(MSR_EFER);
        msrval |= MSR_FLAG_EFER_SCE;
        wrmsr(MSR_EFER, msrval);

        msrval  = (uint64_t)(uintptr_t)fast_amd_entry;
        msrval |= (uint64_t)SEG_SELECTOR(GDT_KERNEL_CODE, RPL_KERNEL)   << 32;
        msrval |= (uint64_t)SEG_SELECTOR(GDT_USER_CODE,   RPL_USER)     << 48;

        wrmsr(MSR_STAR, msrval);
    }
    else if(cpu_has_feature(CPUINFO_FEATURE_SYSENTER)) {
        syscall_implementation = JINUE_I686_HOWSYSCALL_FAST_INTEL;

        wrmsr(MSR_IA32_SYSENTER_CS,  SEG_SELECTOR(GDT_KERNEL_CODE, RPL_KERNEL));
        wrmsr(MSR_IA32_SYSENTER_EIP, (uint64_t)(uintptr_t)fast_intel_entry);

        /* kernel stack address is set when switching thread context */
        wrmsr(MSR_IA32_SYSENTER_ESP, (uint64_t)(uintptr_t)NULL);
    }
    else {
        syscall_implementation = JINUE_I686_HOWSYSCALL_INTERRUPT;
    }
}

static void get_kernel_exec_file(exec_file_t *kernel, const bootinfo_t *bootinfo) {
    kernel->start   = bootinfo->kernel_start;
    kernel->size    = bootinfo->kernel_size;

    if(kernel->start == NULL) {
        panic("malformed boot image: no kernel ELF binary");
    }

    if(kernel->size < sizeof(Elf32_Ehdr)) {
        panic("kernel too small to be an ELF binary");
    }

    if(! elf_check(bootinfo->kernel_start)) {
        panic("kernel ELF binary is invalid");
    }
}

static void get_loader_elf(exec_file_t *loader, const bootinfo_t *bootinfo) {
    loader->start   = bootinfo->loader_start;
    loader->size    = bootinfo->loader_size;

    if(loader->start == NULL) {
        panic("malformed boot image: no user space loader ELF binary");
    }

    if(loader->size < sizeof(Elf32_Ehdr)) {
        panic("user space loader too small to be an ELF binary");
    }

    info("Found user space loader with size %" PRIu32 " bytes.", loader->size);
}

static void get_ramdisk(kern_mem_block_t *ramdisk, const bootinfo_t *bootinfo) {
    ramdisk->start  = bootinfo->ramdisk_start;
    ramdisk->size   = bootinfo->ramdisk_size;

    if(ramdisk->start == 0 || ramdisk->size == 0) {
        panic("No initial RAM disk loaded.");
    }
}

void machine_get_loader(exec_file_t *elf) {
    const bootinfo_t *bootinfo = get_bootinfo();
    get_loader_elf(elf, bootinfo);
}

void machine_get_ramdisk(kern_mem_block_t *ramdisk) {
    const bootinfo_t *bootinfo = get_bootinfo();
    get_ramdisk(ramdisk, bootinfo);
}

void machine_init_logging(const config_t *config) {
    init_uart16550a(config);
    vga_init(config);
}

void machine_init(const config_t *config) {
    /* Validate the boot information structure before using it. */
    (void)check_bootinfo(true);

    detect_cpu_features();

    const bootinfo_t *bootinfo = get_bootinfo();

    check_memory(bootinfo);

    move_kernel_at_16mb(bootinfo);

    boot_alloc_t boot_alloc;
    boot_alloc_init(&boot_alloc, bootinfo);

    bool pae_enabled = maybe_enable_pae(&boot_alloc, bootinfo, config);

    /* Re-initialize the boot page allocator to allocate following the kernel
     * image at 16MB rather than at 1MB, now that the kernel has been moved
     * there.
     *
     * Do this after enabling PAE: we want the temporary PAE page tables to be
     * allocated after 1MB because we won't need them anymore once the final
     * address space is created. */
    boot_alloc_reinit_at_16mb(&boot_alloc);

    /* allocate per-CPU data
     *
     * We need to ensure that the Task State Segment (TSS) contained in this
     * memory block does not cross a page boundary. */
    assert(sizeof(percpu_t) < PERCPU_DATA_ALIGNMENT);
    percpu_t *cpu_data = boot_heap_alloc(&boot_alloc, percpu_t, PERCPU_DATA_ALIGNMENT);

    /* initialize per-CPU data */
    init_percpu_data(cpu_data);

    /* Initialize interrupt descriptor table (IDT) */
    init_idt();

    /* load segment selectors */
    load_selectors(cpu_data);

    /* We must look for the ACPI RDSP while the relevant memory is still
     * identity mapped before we switch to the initial address space. */
    find_acpi_rsdp();
    find_mp();

    /* This must be done before we switch to the new address space because only
     * the boot allocator can allocate multiple consecutive pages. */
    memory_initialize_array(&boot_alloc, bootinfo);

    exec_file_t kernel;
    get_kernel_exec_file(&kernel, bootinfo);

    addr_space_t *addr_space = pmap_create_initial_addr_space(&kernel, &boot_alloc, bootinfo);

    /* switch to new address space */
    pmap_switch_addr_space(addr_space);

    enable_global_pages();

    /* This should be done early after calling pmap_switch_addr_space()
     * because, until it's done, any attempt to log anything to VGA will
     * result in a kernel panic caused by a kernel page fault (and the
     * panic handler itself will then attempt to log). */
    remap_text_video_memory();

    /* Transfer the remaining pages to the run-time page allocator. */
    boot_alloc_reinit_at_klimit(&boot_alloc);
    initialize_page_allocator(&boot_alloc);

    init_acpi();
    
    report_acpi();

    init_mp();

    /* create slab cache to allocate PDPTs
     *
     * This must be done after the global page allocator has been initialized
     * because the slab allocator needs to allocate a slab to allocate the new
     * slab cache on the slab cache cache.
     *
     * This must be done before the first time pmap_create_addr_space() is
     * called, which happens when the first process is created. */
    if(pae_enabled) {
        pae_create_pdpt_cache();
    }

    /* Initialize programmable interrupt_controller. */
    pic8259_init();

    /* Initialize programmable interval timer and enable timer interrupt.
     *
     * Interrupts are disabled during initialization so the CPU won't actually
     * be interrupted until the first user space thread starts. */
    pit8253_init();
    pic8259_unmask(IRQ_TIMER);

    /* choose a system call implementation */
    select_syscall_implementation();
}
