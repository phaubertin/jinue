#include <alloc.h>
#include <assert.h>
#include <boot.h>
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


void kernel(void) {
	kinit();	
	idle();
	
	panic("idle() returned.");	
}

void kinit(void) {
	pte_t *page_table1, *page_table2, *page_directory;
	unsigned long idx, idy;
	unsigned long temp;
	
	/* say hello */
	vga_init();	
	printk("Kernel started.\n");

	/** ASSERTION: we assume the kernel starts on a page boundary */
	assert( PAGE_OFFSET_OF( (unsigned int)kernel_start ) == 0 );
	
	/** ASSERTION: we assume kernel_start is aligned with a page directory entry boundary */
	assert( PAGE_TABLE_OFFSET_OF(PAGE_TABLES_ADDR) == 0 );
	assert( PAGE_OFFSET_OF(PAGE_TABLES_ADDR) == 0 );
	
	/** ASSERTION: we assume kernel_start is aligned with a page directory entry boundary */
	assert( PAGE_TABLE_OFFSET_OF(PAGE_DIRECTORY_ADDR) == 0 );
	assert( PAGE_OFFSET_OF(PAGE_DIRECTORY_ADDR) == 0 );
	
	printk("Kernel size is %u bytes.\n", kernel_size);
	
	/* initialize data structures for caches and the global virtual page allocator */
	slab_create(
		&global_pool_cache,
		&global_pool,
		sizeof(vm_link_t),
		VM_FLAG_KERNEL);
		
	vm_create_pool(&global_pool, &global_pool_cache);
	
	slab_create(&process_slab_cache, &global_pool,
		sizeof(process_t), VM_FLAG_KERNEL);

	/* allocate one page for page directory template just after the 
	   kernel image. Since paging is not yet activated, virtual and
	   physical address are the same. */
	page_directory_template = (pte_t *)kernel_region_top;
	kernel_region_top += PAGE_SIZE;
	
	/* allocate page tables for kernel data/code region (0..KLIMIT) and add
	   relevant entries to page directory template */
	for(idx = 0; idx < PAGE_DIRECTORY_OFFSET_OF(KLIMIT); ++idx) {
		page_table1 = kernel_region_top;
		kernel_region_top += PAGE_SIZE;
		
		page_directory_template[idx] = (pte_t)page_table1 | VM_FLAG_PRESENT | VM_FLAG_KERNEL;
		
		for(idy = 0; idy < PAGE_TABLE_ENTRIES; ++idy) {
			page_table1[idy] = 0;
		}
	}
	
	while(idx < PAGE_TABLE_ENTRIES) {
		page_directory_template[idx] = 0;
		++idx;
	}
	
	/* allocate and fill content of a page directory and two page tables
	   for the creation of the address space of the first process (idle) */
	page_directory = kernel_region_top;
	kernel_region_top += PAGE_SIZE;
	
	page_table1 = kernel_region_top;
	kernel_region_top += PAGE_SIZE;
	
	page_table2 = kernel_region_top;
	kernel_region_top += PAGE_SIZE;
	
	for(idx = 0; idx < PAGE_TABLE_ENTRIES; ++idx) {
		page_directory[idx] = page_directory_template[idx];
	}
	
	page_directory[PAGE_DIRECTORY_OFFSET_OF(PAGE_TABLES_ADDR)] =
		(pte_t)page_table1 | VM_FLAG_PRESENT | VM_FLAG_KERNEL;
		
	page_directory[PAGE_DIRECTORY_OFFSET_OF(PAGE_DIRECTORY_ADDR)] =
		(pte_t)page_table2 | VM_FLAG_PRESENT | VM_FLAG_KERNEL;
	
	for(idx = 0; idx < PAGE_TABLE_ENTRIES; ++idx) {
		page_table1[idx] = page_directory[idx];
	}
	
	page_table2[0] = (pte_t)page_directory;
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
	
	/* TODO: perform 1:1 mapping */
	
	/* activate paging */
	set_cr3( (unsigned long)page_directory );
	
	/*temp = get_cr0();
	temp |= (1 << X86_FLAG_PG);
	set_cr0x(temp);*/
	
	
	/* notes, not actual code
	
	vm_page_directory_index = PAGE_DIRECTORY_OFFSET_OF(x);
	vm_page_tables_index    = PAGE_DIRECTORY_OFFSET_OF(x);
	
	PAGE_TABLES_ADDR
	PAGE_DIRECTORY_ADDR*/
	
	/* initialize page frame allocator */
	alloc_init();
}

void idle(void) {
	while(1) {}
}

