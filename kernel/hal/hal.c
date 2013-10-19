#include <jinue/syscall.h>
#include <jinue/types.h>
#include <assert.h>
#include <boot.h>
#include <bootmem.h>
#include <cpu.h>
#include <hal.h>
#include <interrupt.h>
#include <irq.h>
#include <kernel.h>
#include <panic.h>
#include <pfalloc.h>
#include <printk.h>
#include <syscall.h>
#include <util.h>
#include <vga.h>
#include <vm.h>
#include <vm_alloc.h>
#include <vm_x86.h>
#include <x86.h>
#include <stdint.h>


/** if non_zero, we are executing in the kernel */
int in_kernel;

/** size of the kernel image */
size_t kernel_size;

/** address of top of kernel image (kernel_start + kernel_size) */
addr_t kernel_top;

/** top of region of memory mapped 1:1 (kernel image plus some pages for
    data structures allocated during initialization) */
addr_t kernel_region_top;

/** address of kernel stack */
addr_t kernel_stack;

static vm_alloc_t __global_page_allocator;

/** global page allocator (region 0..KLIMIT) */
vm_alloc_t *global_page_allocator;


void hal_start(void) {
	pte_t *page_directory;
	addr_t addr;
	addr_t stack;
    cpu_data_t *cpu_data;
    pseudo_descriptor_t *pseudo;
    unsigned int idx;
	unsigned long temp;
	unsigned long flags;
	unsigned long long msrval;
	pfaddr_t *page_stack_buffer;
    addr_t  boot_heap_old;
    
    /** ASSERTION: we assume the kernel starts on a page boundary */
	assert( page_offset_of( (unsigned int)kernel_start ) == 0 );
	
	/* pfalloc() should not be called yet -- use pfalloc_early() instead */
	use_pfalloc_early = true;
	
	/* we are in the kernel */
	in_kernel = 1;
    
	/* initialize VGA and say hello */
	vga_init();
	
	printk("Kernel started.\n");
	printk("Kernel size is %u bytes.\n", kernel_size);
    
	/* get cpu info */
	cpu_detect_features();
	
	printk("Processor vendor is %s.\n", cpu_vendor_name[cpu_vendor]);
	
	if(cpu_features & CPU_FEATURE_LOCAL_APIC) {
		printk("Processor has local APIC.\n");
	}
	
	if(cpu_features & CPU_FEATURE_PAE) {
		printk("Processor supports Physical Address Extension (PAE).\n");
	}
		
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
	
	ltr( SEG_SELECTOR(GDT_TSS, 0) );
	
	/* initialize IDT */
	for(idx = 0; idx < IDT_VECTOR_COUNT; ++idx) {
		/* get address, which is already stored in the IDT entry */
		addr  = (addr_t)*(word32_t *)&idt[idx];
		
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
    
    /* initialize virtual memory management */
    vm_init_x86();
    
    /* initialize the page frame allocator */
	page_stack_buffer = (pfaddr_t *)pfalloc_early();
	init_pfcache(&global_pfcache, page_stack_buffer);
	
	for(idx = 0; idx < KERNEL_PAGE_STACK_INIT; ++idx) {
		pffree( PTR_TO_PFADDR( pfalloc_early() ) );
	}
		
    /* create first address space */
    page_directory = vm_create_addr_space_early();
	
	/** below this point, it is no longer safe to call pfalloc_early() */
	use_pfalloc_early = false;
	
	/* perform 1:1 mapping of kernel image and data
	
	   note: page tables for memory region (0..KLIMIT) are contiguous
	         in physical memory */
	for(addr = kernel_start; addr < kernel_region_top; addr += PAGE_SIZE) {
		vm_map_early((addr_t)addr, (addr_t)addr, VM_FLAG_KERNEL | VM_FLAG_READ_WRITE, page_directory);
	}
    
    /* enable paging */
	set_cr3( (uint32_t)page_directory );
    
	temp = get_cr0();
	temp |= X86_FLAG_PG;
	set_cr0x(temp);
    
    /* perform 1:1 mapping of text video memory */
	vm_map((addr_t)VGA_TEXT_VID_BASE,           PTR_TO_PFADDR(VGA_TEXT_VID_BASE),           VM_FLAG_KERNEL | VM_FLAG_READ_WRITE);
    vm_map((addr_t)VGA_TEXT_VID_BASE+PAGE_SIZE, PTR_TO_PFADDR(VGA_TEXT_VID_BASE+PAGE_SIZE), VM_FLAG_KERNEL | VM_FLAG_READ_WRITE);
	
	/* create system memory map */
	bootmem_init();
    
	/** TODO: handle hole due to video memory mapping */
    
    /* initialize global page allocator (region 0..KLIMIT)
	  
	   note: we skip the first page (i.e. actually allocate the region PAGE_SIZE..KLIMIT)
	         for two reasons:
				- We want null pointer deferences to generate a page fault instead
				  being more or less silently ignored (read) or overwriting something
				  potentially important (write).
				- We want to ensure nothing interesting (e.g. address space
				  management data structures) can have NULL as its valid
				  address.*/
	global_page_allocator = &__global_page_allocator;
	vm_alloc_init_piecewise(global_page_allocator, (addr_t)PAGE_SIZE,  (addr_t)kernel_start, (addr_t)KLIMIT);
	vm_alloc_add_region(global_page_allocator, (addr_t)kernel_region_top, (addr_t)KLIMIT);
	
	/* choose system call method */
	syscall_method = SYSCALL_METHOD_INTR;
	
	if(cpu_features & CPU_FEATURE_SYSENTER) {
		syscall_method = SYSCALL_METHOD_FAST_INTEL;
		
		wrmsr(MSR_IA32_SYSENTER_CS,  GDT_KERNEL_CODE * 8);
		wrmsr(MSR_IA32_SYSENTER_EIP, (unsigned long long)(unsigned long)fast_intel_entry);
		wrmsr(MSR_IA32_SYSENTER_ESP, (unsigned long long)(unsigned long)stack);
	}
	
	if(cpu_features & CPU_FEATURE_SYSCALL) {
		syscall_method = SYSCALL_METHOD_FAST_AMD;
		
		msrval  = rdmsr(MSR_EFER);
		msrval |= MSR_FLAG_STAR_SCE;
		wrmsr(MSR_EFER, msrval);
		
		msrval  = (unsigned long long)(unsigned long)fast_amd_entry;
		msrval |= (unsigned long long)SEG_SELECTOR(GDT_KERNEL_CODE, 0) << 32;
		msrval |= (unsigned long long)SEG_SELECTOR(GDT_USER_CODE,   3) << 48;
		
		wrmsr(MSR_STAR, msrval);
	}
	
	/* set system call dispatch function to default */
	set_syscall_funct(NULL);
	
	kmain();
}
