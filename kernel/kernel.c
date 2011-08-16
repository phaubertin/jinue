#include <jinue/syscall.h>
#include <alloc.h>
#include <assert.h>
#include <boot.h>
#include <bootmem.h>
#include <cpu.h>
#include <elf.h>
#include <interrupt.h>
#include <irq.h>
#include <kernel.h> /* includes stddef.h */
#include <panic.h>
#include <printk.h>
#include <process.h>
#include <stddef.h>
#include <syscall.h>
#include <thread.h>
#include <vga.h>
#include <vm.h>
#include <vm_alloc.h>
#include <x86.h>


/** size of the kernel image */
size_t kernel_size;

/** address of top of kernel image (kernel_start + kernel_size) */
addr_t kernel_top;

/** top of region of memory mapped 1:1 (kernel image plus some pages for
    data structures allocated during initialization) */
addr_t kernel_region_top;

/** process descriptor for first process (idle) */
process_t idle_process;

/** address of kernel stack */
addr_t kernel_stack;


void kernel(void) {
	kinit();	
	idle();
	
	panic("idle() returned.");	
}

void kinit(void) {
	pte_t *page_table, *page_directory;
	pte_t *pte;
	addr_t addr;
	gdt_t  gdt;
	tss_t *tss;
	addr_t stack;
	gdt_info_t *gdt_info;
	idt_info_t *idt_info;
	unsigned long idx, idy;
	unsigned long temp;
	unsigned long flags;
	unsigned long *ulptr;
	unsigned long long msrval;
	physaddr_t *page_stack_buffer;


	/** ASSERTION: we assume the kernel starts on a page boundary */
	assert( PAGE_OFFSET_OF( (unsigned int)kernel_start ) == 0 );
	
	/** ASSERTION: we assume kernel_start is aligned with a page directory entry boundary */
	assert( PAGE_TABLE_OFFSET_OF(PAGE_TABLES_ADDR) == 0 );
	assert( PAGE_OFFSET_OF(PAGE_TABLES_ADDR) == 0 );
	
	/** ASSERTION: we assume kernel_start is aligned with a page directory entry boundary */
	assert( PAGE_TABLE_OFFSET_OF(PAGE_DIRECTORY_ADDR) == 0 );
	assert( PAGE_OFFSET_OF(PAGE_DIRECTORY_ADDR) == 0 );
		
	/* alloc_page() should not be called yet -- use alloc_page_early() instead */
	__alloc_page         = &do_not_call;
	use_alloc_page_early = true;
	
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
		
	/* allocate new kernel stack */
	stack = alloc_page_early();
	stack += PAGE_SIZE;
	
	/* allocate page stack */
	page_stack_buffer = (physaddr_t *)alloc_page_early();
	
	/* allocate space for new GDT and TSS */
	gdt_info = (gdt_info_t *)alloc_page_early();
	idt_info = (idt_info_t *)gdt_info;
	gdt = (gdt_t)&gdt_info[2];
	tss = (tss_t *)&gdt[GDT_END];
	
	/* initialize GDT */
	gdt[GDT_NULL] = SEG_DESCRIPTOR(0, 0, 0);
	gdt[GDT_KERNEL_CODE] =
		SEG_DESCRIPTOR(0, 0xfffff,                      SEG_TYPE_CODE  | SEG_FLAG_KERNEL | SEG_FLAG_NORMAL);
	gdt[GDT_KERNEL_DATA] =
		SEG_DESCRIPTOR(0, 0xfffff,                      SEG_TYPE_DATA  | SEG_FLAG_KERNEL | SEG_FLAG_NORMAL);
	gdt[GDT_USER_CODE] =
		SEG_DESCRIPTOR(0, 0xfffff,                      SEG_TYPE_CODE  | SEG_FLAG_USER   | SEG_FLAG_NORMAL);
	gdt[GDT_USER_DATA] =
		SEG_DESCRIPTOR(0, 0xfffff,                      SEG_TYPE_DATA  | SEG_FLAG_USER   | SEG_FLAG_NORMAL);
	gdt[GDT_TSS] =
		SEG_DESCRIPTOR((unsigned long)tss, TSS_LIMIT-1, SEG_TYPE_TSS   | SEG_FLAG_KERNEL | SEG_FLAG_TSS);
	gdt[GDT_TSS_DATA] =
		SEG_DESCRIPTOR((unsigned long)tss, TSS_LIMIT-1, SEG_TYPE_DATA  | SEG_FLAG_KERNEL | SEG_FLAG_32BIT | SEG_FLAG_IN_BYTES | SEG_FLAG_NOSYSTEM | SEG_FLAG_PRESENT);
	
	gdt_info->addr  = gdt;
	gdt_info->limit = GDT_END * 8 - 1;
	
	lgdt(gdt_info);
	set_cs( SEG_SELECTOR(GDT_KERNEL_CODE, 0) );
	set_ss( SEG_SELECTOR(GDT_KERNEL_DATA, 0) );
	set_data_segments( SEG_SELECTOR(GDT_KERNEL_DATA, 0) );
	
	/* initialize TSS */
	ulptr = (unsigned long *)tss;
	for(idx = 0; idx < TSS_LIMIT / sizeof(unsigned long); ++idx) {
		ulptr[idx] = 0;
	}
 	
	tss->ss0 = SEG_SELECTOR(GDT_KERNEL_DATA, 0);
	tss->ss1 = SEG_SELECTOR(GDT_KERNEL_DATA, 0);
	tss->ss2 = SEG_SELECTOR(GDT_KERNEL_DATA, 0);

	tss->esp0 = stack;
	tss->esp1 = stack;
	tss->esp2 = stack;
	
	ltr( SEG_SELECTOR(GDT_TSS, 0) );
	
	/* initialize IDT */
	for(idx = 0; idx < IDT_VECTOR_COUNT; ++idx) {
		/* get address, which is already stored in the IDT entry */
		addr  = (addr_t)*(unsigned long *)&idt[idx];
		
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
			(unsigned long)addr,
			flags,
			NULL );
	}
	
	idt_info->addr  = idt;
	idt_info->limit = IDT_VECTOR_COUNT * sizeof(seg_descriptor_t) - 1;
	lidt(idt_info);
	
	/* choose system call mechanism */
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
		
	/* Allocate the first page directory. Since paging is not yet
	   activated, virtual and physical addresses are the same.  */
	page_directory = (pte_t *)alloc_page_early();
	
	/* allocate page tables for kernel data/code region (0..PLIMIT) and add
	   relevant entries to page directory */
	for(idx = 0; idx < PAGE_DIRECTORY_OFFSET_OF(PLIMIT); ++idx) {
		pte = (pte_t *)alloc_page_early();		
		page_directory[idx] = (pte_t)pte | VM_FLAG_PRESENT | VM_FLAG_KERNEL | VM_FLAG_READ_WRITE;
		
		for(idy = 0; idy < PAGE_TABLE_ENTRIES; ++idy) {
			pte[idy] = 0;
		}
	}
	
	while(idx < PAGE_TABLE_ENTRIES) {
		page_directory[idx] = 0;
		++idx;
	}
	
	/* map page directory */
	vm_map_early((addr_t)PAGE_DIRECTORY_ADDR, (physaddr_t)page_directory, VM_FLAGS_PAGE_TABLE, page_directory);
		
	/* map page tables */
	for(idx = 0, addr = (addr_t)PAGE_TABLES_ADDR; idx < PAGE_DIRECTORY_OFFSET_OF(PLIMIT); ++idx, addr += PAGE_SIZE)	{
		page_table = (pte_t *)page_directory[idx];
		page_table = (pte_t *)( (unsigned int)page_table & ~PAGE_MASK  );
		
		vm_map_early((addr_t)addr, (physaddr_t)page_table, VM_FLAGS_PAGE_TABLE, page_directory);		
	}
	
	/* perform 1:1 mapping of text video memory */
	vm_map_early((addr_t)VGA_TEXT_VID_BASE,           (physaddr_t)VGA_TEXT_VID_BASE,           VM_FLAG_KERNEL | VM_FLAG_READ_WRITE, page_directory);
	vm_map_early((addr_t)VGA_TEXT_VID_BASE+PAGE_SIZE, (physaddr_t)VGA_TEXT_VID_BASE+PAGE_SIZE, VM_FLAG_KERNEL | VM_FLAG_READ_WRITE, page_directory);
	
	/** below this point, it is no longer safe to call alloc_page_early() */
	use_alloc_page_early = false;
	
	/* perform 1:1 mapping of kernel image and data
	
	   note: page tables for memory region (0..KLIMIT) are contiguous
	         in physical memory */
	for(addr = kernel_start; addr < kernel_region_top; addr += PAGE_SIZE) {
		vm_map_early((addr_t)addr, (physaddr_t)addr, VM_FLAG_KERNEL | VM_FLAG_READ_WRITE, page_directory);
	}
	
	/* initialize boot-time page frame allocator */
	bootmem_init();
	
	/* enable paging */
	set_cr3( (unsigned long)page_directory );
	
	temp = get_cr0();
	temp |= X86_FLAG_PG;
	set_cr0x(temp);
	
	printk("Paging enabled\n");
	
	/* initialize page stack */
	page_stack = &__page_stack;
	init_page_stack(page_stack, page_stack_buffer);
	
	/* create thread control block for first thread */
	current_thread = (thread_t *)boot_heap;
	boot_heap = (thread_t *)boot_heap + 1;
	init_thread(current_thread, stack);
	
	/* load process manager binary */
	elf_load_process_manager();
	
	/* start process manager */
	elf_start_process_manager();
}

void idle(void) {
	while(1) {}
}

