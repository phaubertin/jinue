#include <jinue/syscall.h>
#include <assert.h>
#include <hal/boot.h>
#include <hal/bootmem.h>
#include <hal/cpu.h>
#include <hal/hal.h>
#include <hal/interrupt.h>
#include <hal/irq.h>
#include <hal/kernel.h>
#include <hal/thread.h>
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


/** if non_zero, we are executing in the kernel */
int in_kernel;

/** top of region of memory mapped 1:1 (kernel image plus some pages for
    data structures allocated during initialization) */
addr_t kernel_region_top;

/** Specifies the entry point to use for system calls */
int syscall_method;

void hal_init(void) {
    addr_t addr;
    addr_t               stack;
    cpu_data_t          *cpu_data;
    pseudo_descriptor_t *pseudo;
    unsigned int         idx;
    unsigned int         flags;
    uint64_t             msrval;
    pfaddr_t            *page_stack_buffer;
    addr_t               boot_heap_old;
    
    /* pfalloc() should not be called yet -- use pfalloc_early() instead */
    use_pfalloc_early = true;
    
    /* we are in the kernel */
    in_kernel = 1;
    
    /* initialize VGA and say hello */
    vga_init();
    
    /* We want this call and the assertions below after vga_init() so that if
     * any of them fail, we have a useful error message on screen. */
    (void)boot_info_check(true);
    
    const boot_info_t *boot_info = get_boot_info();

    /** ASSERTION: we assume the image starts on a page boundary */
    assert(page_offset_of(boot_info->image_start) == 0);

    /** ASSERTION: we assume the kernel starts on a page boundary */
    assert(page_offset_of(boot_info->kernel_start) == 0);

    printk("Kernel started.\n");
    printk("Kernel size is %u bytes.\n", boot_info->kernel_size);
    
    /* This must be done before calling bootmem_init() */
    boot_heap = boot_info->boot_heap;

    /* This must be done before calling bootmem_init() or any call to
     * pfalloc_early(). */
    kernel_region_top = boot_info->boot_end;

    /* create system memory map */
    bootmem_init();
    
    /* get cpu info */
    cpu_detect_features();
    
    /* allocate new kernel stack */
    stack = pfalloc_early();
    stack += PAGE_SIZE;
    
    /* allocate per-CPU data
     * 
     * We need to ensure that the Task State Segment (TSS) contained in this
     * memory block does not cross a page boundary. */
    assert(sizeof(cpu_data_t) < CPU_DATA_ALIGNMENT);
    
    boot_heap = ALIGN_END(boot_heap, CPU_DATA_ALIGNMENT);
    
    cpu_data  = (cpu_data_t *)boot_heap;
    boot_heap = (cpu_data_t *)boot_heap + 1;
    
    /* initialize per-CPU data */
    cpu_init_data(cpu_data, stack);
    
    /* allocate pseudo-descriptor for GDT and IDT (temporary allocation) */
    boot_heap_old = boot_heap;
    
    boot_heap = ALIGN_END(boot_heap, sizeof(pseudo_descriptor_t));
    
    pseudo    = (pseudo_descriptor_t *)boot_heap;
    boot_heap = (pseudo_descriptor_t *)boot_heap + 1;
    
    /* load new GDT and TSS */
    pseudo->addr    = (addr_t)&cpu_data->gdt;
    pseudo->limit   = GDT_LENGTH * 8 - 1;
    
    lgdt(pseudo);
    
    set_cs( SEG_SELECTOR(GDT_KERNEL_CODE, 0) );
    set_ss( SEG_SELECTOR(GDT_KERNEL_DATA, 0) );
    set_data_segments( SEG_SELECTOR(GDT_KERNEL_DATA, 0) );
    set_gs( SEG_SELECTOR(GDT_PER_CPU_DATA, 0) );
    
    ltr( SEG_SELECTOR(GDT_TSS, 0) );
    
    /* initialize IDT */
    for(idx = 0; idx < IDT_VECTOR_COUNT; ++idx) {
        /* get address, which is already stored in the IDT entry */
        addr  = (addr_t)(uintptr_t)idt[idx];
        
        /* set interrupt gate flags */
        flags = SEG_TYPE_INTERRUPT_GATE | SEG_FLAG_NORMAL_GATE;
        
        if(idx == SYSCALL_IRQ) {
            flags |= SEG_FLAG_USER;
        }
        else {
            flags |= SEG_FLAG_KERNEL;
        }
        
        /* create interrupt gate descriptor */
        idt[idx] = GATE_DESCRIPTOR(
            SEG_SELECTOR(GDT_KERNEL_CODE, 0),
            addr,
            flags,
            NULL );
    }
    
    pseudo->addr  = (addr_t)idt;
    pseudo->limit = IDT_VECTOR_COUNT * sizeof(seg_descriptor_t) - 1;
    lidt(pseudo);
    
    /* de-allocate pseudo-descriptor */
    boot_heap = boot_heap_old;
    
    /* initialize the page frame allocator */
    page_stack_buffer = (pfaddr_t *)pfalloc_early();
    init_pfcache(&global_pfcache, page_stack_buffer);
    
    for(idx = 0; idx < KERNEL_PAGE_STACK_INIT; ++idx) {
        pffree( PTR_TO_PFADDR( pfalloc_early() ) );
    }
    
    /* initialize virtual memory management, enable paging
     * 
     * below this point, it is no longer safe to call pfalloc_early() */
    vm_boot_init();
    
    /* choose system call method */
    syscall_method = SYSCALL_METHOD_INTR;
    
    if(cpu_has_feature(CPU_FEATURE_SYSENTER)) {
        syscall_method = SYSCALL_METHOD_FAST_INTEL;
        
        wrmsr(MSR_IA32_SYSENTER_CS,  GDT_KERNEL_CODE * 8);
        wrmsr(MSR_IA32_SYSENTER_EIP, (uint64_t)(uintptr_t)fast_intel_entry);

        /* kernel stack address is set when switching thread context */
        wrmsr(MSR_IA32_SYSENTER_ESP, (uint64_t)(uintptr_t)NULL);
    }
    
    if(cpu_has_feature(CPU_FEATURE_SYSCALL)) {
        syscall_method = SYSCALL_METHOD_FAST_AMD;
        
        msrval  = rdmsr(MSR_EFER);
        msrval |= MSR_FLAG_STAR_SCE;
        wrmsr(MSR_EFER, msrval);
        
        msrval  = (uint64_t)(uintptr_t)fast_amd_entry;
        msrval |= (uint64_t)SEG_SELECTOR(GDT_KERNEL_CODE, 0) << 32;
        msrval |= (uint64_t)SEG_SELECTOR(GDT_USER_CODE,   3) << 48;
        
        wrmsr(MSR_STAR, msrval);
    }
}
