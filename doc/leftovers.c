/* --- From kinit --------------------------------------------------- */

	/* allocate two more page table and do some stuff */
	page_table1 = (pte_t *)early_alloc_page();
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
	}
	
	
/* --- Also from kinit ---------------------------------------------- */

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


/* --- Removed this debugging code from kinit() --------------------- */

	/** TODO: remove */
	printk("boot data: 0x%x\n", (unsigned long)get_boot_data() );
	
	printk("page directory (0x%x):\n", (unsigned long)page_directory);
	printk("  0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",
		(unsigned long)page_directory[0],
		(unsigned long)page_directory[1],
		(unsigned long)page_directory[2],
		(unsigned long)page_directory[3],
		(unsigned long)page_directory[4],
		(unsigned long)page_directory[5],
		(unsigned long)page_directory[6]
		);
	
	if(PAGE_DIRECTORY_OFFSET_OF(kernel_start) != 0) {
		printk("OOPS: PAGE_DIRECTORY_OFFSET_OF(kernel_start) != 0 (%u)\n",
			PAGE_DIRECTORY_OFFSET_OF(kernel_start));
	}
	
	if(PAGE_TABLE_OFFSET_OF(kernel_start) != 256) {
		printk("PAGE_TABLE_OFFSET_OF(kernel_start) != 256 (%u)\n",
			PAGE_TABLE_OFFSET_OF(kernel_start));
	}
	
	page_table1 = 
		(pte_t *)page_directory[0];
	page_table1 = (pte_t *)( (unsigned int)page_table1 & ~PAGE_MASK  );
	pte = (pte_t *)&page_table1[250];
	printk("Page table 0 (0x%x) offset 250 (0x%x):\n", (unsigned long)page_table1, (unsigned long) pte);
	
	for(idx = 0; idx < 42; ++idx) {
		if(idx % 7 == 0) {
			printk("  0x%x ", (unsigned long)pte[idx]);
		}
		else if(idx % 7 == 6) {
			printk("0x%x\n", (unsigned long)pte[idx]);
		}
		else {
			printk("0x%x ", (unsigned long)pte[idx]);
		}
	}
	
	page_table1 = 
		(pte_t *)page_directory[4];
	page_table1 = (pte_t *)( (unsigned int)page_table1 & ~PAGE_MASK  );
	printk("page table 4 (0x%x):\n", (unsigned long)page_table1);
	printk("  0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",
		(unsigned long)page_table1[0],
		(unsigned long)page_table1[1],
		(unsigned long)page_table1[2],
		(unsigned long)page_table1[3],
		(unsigned long)page_table1[4],
		(unsigned long)page_table1[5],
		(unsigned long)page_table1[6]
		);
	
	page_table1 = 
		(pte_t *)page_directory[5];
	page_table1 = (pte_t *)( (unsigned int)page_table1 & ~PAGE_MASK  );
	printk("page table 5 (0x%x):\n", (unsigned long)page_table1);
	printk("  0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",
		(unsigned long)page_table1[0],
		(unsigned long)page_table1[1],
		(unsigned long)page_table1[2],
		(unsigned long)page_table1[3],
		(unsigned long)page_table1[4],
		(unsigned long)page_table1[5],
		(unsigned long)page_table1[6]
		);
	
	/** TODO: /remove */
	
	
	/** TODO: remove */
	printk("kernel_region_top on entry: 0x%x\n", (unsigned long)kernel_region_top );
