#include <alloc.h>
#include <assert.h>
#include <boot.h>
#include <bootmem.h>
#include <elf.h>
#include <kernel.h> /* includes stddef.h */
#include <panic.h>
#include <printk.h>
#include <process.h>
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
	pte_t *page_table1, *page_table2, *page_directory;
	pte_t *pte;
	addr_t addr;
	gdt_t gdt;
	gdt_info_t *gdt_info;
	unsigned long idx, idy;
	unsigned long temp;
	x86_regs_t regs;
	char str[13];
	
	
	/** ASSERTION: we assume the kernel starts on a page boundary */
	assert( PAGE_OFFSET_OF( (unsigned int)kernel_start ) == 0 );
	
	/** ASSERTION: we assume kernel_start is aligned with a page directory entry boundary */
	assert( PAGE_TABLE_OFFSET_OF(PAGE_TABLES_ADDR) == 0 );
	assert( PAGE_OFFSET_OF(PAGE_TABLES_ADDR) == 0 );
	
	/** ASSERTION: we assume kernel_start is aligned with a page directory entry boundary */
	assert( PAGE_TABLE_OFFSET_OF(PAGE_DIRECTORY_ADDR) == 0 );
	assert( PAGE_OFFSET_OF(PAGE_DIRECTORY_ADDR) == 0 );
		
	
	/* say hello */
	vga_init();	
	printk("Kernel started.\n");
	printk("Kernel size is %u bytes.\n", kernel_size);
	if(proc_elf == ELF_MAGIC) {
		printk("Found process manager ELF binary at: 0x%x\n", (unsigned int)&proc_elf);
	}
	
	/* alloc_page() should not be called yet */
	__alloc_page = &do_not_call;
	
	
	/* get cpu info */
	regs.eax = 0;
	(void)cpuid(&regs);
	*(unsigned long *)&str[0] = regs.ebx;
	*(unsigned long *)&str[4] = regs.edx;
	*(unsigned long *)&str[8] = regs.ecx;
	str[12] = '\0';
	
	printk("Processor is a: %s\n", str);
	
	
	/* setup a new GDT */
	gdt_info = (gdt_info_t *)early_alloc_page();
	gdt = (gdt_t)&gdt_info[2];
	
	gdt[GDT_NULL] = SEG_DESCRIPTOR(0, 0, 0);
	gdt[GDT_KERNEL_CODE] =
		SEG_DESCRIPTOR(0, 0xfffff, SEG_TYPE_CODE | SEG_FLAG_KERNEL | SEG_FLAG_NORMAL);
	gdt[GDT_KERNEL_DATA] =
		SEG_DESCRIPTOR(0, 0xfffff, SEG_TYPE_DATA | SEG_FLAG_KERNEL | SEG_FLAG_NORMAL);
	gdt[GDT_USER_CODE] =
		SEG_DESCRIPTOR(0, 0xfffff, SEG_TYPE_CODE | SEG_FLAG_USER   | SEG_FLAG_NORMAL);
	gdt[GDT_USER_DATA] =
		SEG_DESCRIPTOR(0, 0xfffff, SEG_TYPE_DATA | SEG_FLAG_USER   | SEG_FLAG_NORMAL);
	
	gdt_info->addr  = gdt;
	gdt_info->limit = GDT_END * 8 - 1;
	
	lgdt(gdt_info);
	set_cs( SEG_SELECTOR(GDT_KERNEL_CODE, 0) );
	set_ss( SEG_SELECTOR(GDT_KERNEL_DATA, 0) );
	set_data_segments( SEG_SELECTOR(GDT_KERNEL_DATA, 0) );
		
	/* Allocate page directory for the process manager. Since paging is
	 * not yet activated, virtual and physical addresses are the same. */
	page_directory = (pte_t *)early_alloc_page();
	
	/* allocate page tables for kernel data/code region (0..KLIMIT) and add
	   relevant entries to page directory */
	for(idx = 0; idx < PAGE_DIRECTORY_OFFSET_OF(KLIMIT); ++idx) {
		pte = (pte_t *)early_alloc_page();		
		page_directory[idx] = (pte_t)pte | VM_FLAG_PRESENT | VM_FLAG_KERNEL | VM_FLAG_READ_WRITE;
		
		for(idy = 0; idy < PAGE_TABLE_ENTRIES; ++idy) {
			pte[idy] = 0;
		}
	}
	
	while(idx < PAGE_TABLE_ENTRIES) {
		page_directory[idx] = 0;
		++idx;
	}
	
	/* perform 1:1 mapping of text video memory */
	page_table1 = 
		(pte_t *)page_directory[ PAGE_DIRECTORY_OFFSET_OF(VGA_TEXT_VID_BASE) ];
	page_table1 = (pte_t *)( (unsigned int)page_table1 & ~PAGE_MASK  );
	
	pte = (pte_t *)&page_table1[ PAGE_TABLE_OFFSET_OF(VGA_TEXT_VID_BASE) ];
	*pte = (pte_t)VGA_TEXT_VID_BASE | VM_FLAG_PRESENT | VM_FLAG_KERNEL | VM_FLAG_READ_WRITE;
	++pte;
	*pte = (pte_t)VGA_TEXT_VID_BASE | VM_FLAG_PRESENT | VM_FLAG_KERNEL | VM_FLAG_READ_WRITE;
	
	/** below this point, it is no longer safe to call early_alloc_page() */
	
	/* perform 1:1 mapping of kernel image and data
	
	   note: page tables for memory region (0..KLIMIT) are contiguous
	         in memory */
	page_table1 =
		(pte_t *)page_directory[ PAGE_DIRECTORY_OFFSET_OF(kernel_start) ];
	page_table1 = (pte_t *)( (unsigned int)page_table1 & ~PAGE_MASK  );
	
	pte = (pte_t *)&page_table1[ PAGE_TABLE_OFFSET_OF(kernel_start) ];
	
	for(addr = kernel_start; addr < kernel_region_top; addr += PAGE_SIZE) {
		*pte = (pte_t)addr | VM_FLAG_PRESENT | VM_FLAG_KERNEL | VM_FLAG_READ_WRITE;
		++pte;
	}
	
	/* allocate two more page table and do some stuff */
	/*page_table1 = (pte_t *)early_alloc_page();
	page_table2 = (pte_t *)early_alloc_page();
		
	page_directory[PAGE_DIRECTORY_OFFSET_OF(PAGE_TABLES_ADDR)] =
		(pte_t)page_table1 | VM_FLAG_PRESENT | VM_FLAG_USER | VM_FLAG_READ_WRITE;
		
	page_directory[PAGE_DIRECTORY_OFFSET_OF(PAGE_DIRECTORY_ADDR)] =
		(pte_t)page_table2 | VM_FLAG_PRESENT | VM_FLAG_USER | VM_FLAG_READ_WRITE;
	
	page_table1[PAGE_DIRECTORY_OFFSET_OF(PAGE_TABLES_ADDR)] =
		(pte_t)page_table1 | VM_FLAG_PRESENT | VM_FLAGS_PAGE_TABLE;
		
	page_table1[PAGE_DIRECTORY_OFFSET_OF(PAGE_DIRECTORY_ADDR)] =
		(pte_t)page_table2 | VM_FLAG_PRESENT | VM_FLAGS_PAGE_TABLE;
	
	page_table2[0] = (pte_t)page_directory | VM_FLAG_PRESENT | VM_FLAGS_PAGE_TABLE;
	for(idx = 1; idx < PAGE_TABLE_ENTRIES; ++idx) {
		page_table2[idx] = 0;
	}*/
	
	/* initialize boot-time page frame allocator */
	bootmem_init();
	
	/* activate paging */
	set_cr3( (unsigned long)page_directory );
	
	temp = get_cr0();
	temp |= X86_FLAG_PG;
	set_cr0x(temp);
	
	printk("Still here.\n");
	
	/*idx = *(unsigned long *)0x80000000;*/
	
#if 0
	/* initialize data structures for caches and the global virtual page allocator */
	slab_create(
		&global_pool_cache,
		&global_pool,
		sizeof(vm_link_t),
		VM_FLAG_KERNEL);
		
	vm_create_pool(&global_pool, &global_pool_cache);
	
	slab_create(&process_slab_cache, &global_pool,
		sizeof(process_t), VM_FLAG_KERNEL);

	
	
	/* allocate and fill content of a page directory and two page tables
	   for the creation of the address space of the first process (idle) */
	page_directory = kernel_region_top;
	kernel_region_top += PAGE_SIZE;
	
	page_table1 = kernel_region_top;
	kernel_region_top += PAGE_SIZE;
	
	page_table2 = kernel_region_top;
	kernel_region_top += PAGE_SIZE;
	
	for(idx = 0; idx < PAGE_TABLE_ENTRIES; ++idx) {
		temp = page_directory_template[idx];
		page_directory[idx] = temp;
		page_table1[idx]    = temp;
	}
	
	page_directory[PAGE_DIRECTORY_OFFSET_OF(PAGE_TABLES_ADDR)] =
		(pte_t)page_table1 | VM_FLAG_PRESENT | VM_FLAG_USER | VM_FLAG_READ_WRITE;
		
	page_directory[PAGE_DIRECTORY_OFFSET_OF(PAGE_DIRECTORY_ADDR)] =
		(pte_t)page_table2 | VM_FLAG_PRESENT | VM_FLAG_USER | VM_FLAG_READ_WRITE;
	
	page_table1[PAGE_DIRECTORY_OFFSET_OF(PAGE_TABLES_ADDR)] =
		(pte_t)page_table1 | VM_FLAG_PRESENT | VM_FLAGS_PAGE_TABLE;
		
	page_table1[PAGE_DIRECTORY_OFFSET_OF(PAGE_DIRECTORY_ADDR)] =
		(pte_t)page_table2 | VM_FLAG_PRESENT | VM_FLAGS_PAGE_TABLE;
	
	page_table2[0] = (pte_t)page_directory | VM_FLAG_PRESENT | VM_FLAGS_PAGE_TABLE;
	for(idx = 1; idx < PAGE_TABLE_ENTRIES; ++idx) {
		page_table2[idx] = 0;
	}
	
	/* create process descriptor for first process */
	idle_process.pid = 0;
	next_pid = 1;
	
	idle_process.next = NULL;
	first_process = &idle_process;
	
	idle_process.cr3 = (addr_t)page_directory;
	
	idle_process.name[0] = 'i';
	idle_process.name[1] = 'd';
	idle_process.name[2] = 'l';
	idle_process.name[3] = 'e';
	for(idx = 4; idx < PROCESS_NAME_LENGTH; ++idx) {
		idle_process.name[idx] = 0;
	}
	
	/* perform 1:1 mapping of kernel image and data
	
	   note: page tables for memory region (0..KLIMIT) are contiguous
	         in memory */
	page_table1 =
		(pte_t *)page_directory[ PAGE_DIRECTORY_OFFSET_OF(kernel_start) ];
	page_table1 = (pte_t *)( (unsigned int)page_table1 & ~PAGE_MASK  );
	
	pte =
		(pte_t *)&page_table1[ PAGE_TABLE_OFFSET_OF(kernel_start) ];
	
	for(addr = kernel_start; addr < kernel_region_top; addr += PAGE_SIZE) {
		*pte = (pte_t)addr | VM_FLAG_PRESENT | VM_FLAG_KERNEL;
		++pte;
	}
	
	
	
	
	
	/* activate paging */
	set_cr3( (unsigned long)page_directory );
	
	/*temp = get_cr0();
	temp |= (1 << X86_FLAG_PG);
	set_cr0x(temp);*/
	
	/* initialize page frame allocator */
	alloc_init();
#endif
}

void idle(void) {
	while(1) {}
}

