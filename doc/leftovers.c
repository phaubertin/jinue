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


/* --- From hal/cpu.c,h --------------------------------------------------- */

#define CPU_CACHE_ENTRIES           8

#define CPU_CACHE_ASSOC_DIRECT      1

#define CPU_CACHE_ASSOC_FULL        (-1)


typedef enum {
    CPU_CACHE_NONE    = 0,
    CPU_CACHE_INSTR   = 1,
    CPU_CACHE_DATA    = 2,
    CPU_CACHE_UNIFIED = 3
} cpu_cache_type_t;

typedef struct {
    cpu_cache_type_t    type;
    unsigned int        level;
    unsigned int        size;
    unsigned int        associativity;
    unsigned int        line_size;
} cpu_cache_t;


extern cpu_cache_t cpu_caches[];

void cpu_detect_caches(void);


typedef struct {    
    unsigned int        descriptor;
    cpu_cache_type_t    type;
    unsigned int        level;
    unsigned int        size;
    unsigned int        associativity;
    unsigned int        line_size;
} cpu_intel_cache_descriptor_t;

cpu_cache_t cpu_caches[CPU_CACHE_ENTRIES];

const char *cpu_cache_type_description[] = {
    "(none)",
    "instruction",
    "data",
    "unified"
};

    /** TODO: 40h: "No 2nd-level cache or, if processor contains a valid
     *              2nd-level cache, no 3rd-level cache. "
     *  TODO: 49h:  "3rd-level cache: 4-MB, 16-way set associative,
     *               64-byte line size (Intel Xeon processor MP,
     *               Family 0Fh, Model 06h) "
     *  TODO: ffh:    "CPUID Leaf 2 does not report cache descriptor
     *                   information; use CPUID Leaf 4 to query cache
     *               parameters "  */
const cpu_intel_cache_descriptor_t cpu_intel_cache_descriptors[] = {
    /*  descr.  type                level   size        assoc.  line size */
    {   0x06,   CPU_CACHE_INSTR,    1,        8,             4,     32},
    {   0x08,   CPU_CACHE_INSTR,    1,       16,             4,     32},
    {   0x09,   CPU_CACHE_INSTR,    1,       32,             4,     64},
    {   0x0a,   CPU_CACHE_DATA,     1,        8,             2,     32},
    {   0x0c,   CPU_CACHE_DATA,     1,       16,             4,     32},
    {   0x0d,   CPU_CACHE_DATA,     1,       16,             4,     64},
    {   0x0e,   CPU_CACHE_DATA,     1,       24,             6,     64},
    {   0x21,   CPU_CACHE_UNIFIED,  2,      256,             8,     64},
    {   0x22,   CPU_CACHE_UNIFIED,  3,      512,             4,     64},
    {   0x23,   CPU_CACHE_UNIFIED,  3,        1 * 1024,      8,     64},
    {   0x25,   CPU_CACHE_UNIFIED,  3,        2 * 1024,      8,     64},
    {   0x29,   CPU_CACHE_UNIFIED,  3,        4 * 1024,      8,     64},
    {   0x2c,   CPU_CACHE_DATA,     1,       32,             8,     64},
    {   0x30,   CPU_CACHE_INSTR,    1,       32,             8,     64},
    {   0x41,   CPU_CACHE_UNIFIED,  2,      128,             4,     32},
    {   0x42,   CPU_CACHE_UNIFIED,  2,      256,             4,     32},
    {   0x43,   CPU_CACHE_UNIFIED,  2,      512,             4,     32},
    {   0x44,   CPU_CACHE_UNIFIED,  2,        1 * 1024,      4,     32},
    {   0x45,   CPU_CACHE_UNIFIED,  2,        2 * 1024,      4,     32},
    {   0x46,   CPU_CACHE_UNIFIED,  3,        4 * 1024,      4,     64},
    {   0x47,   CPU_CACHE_UNIFIED,  3,        8 * 1024,      8,     64},
    {   0x48,   CPU_CACHE_UNIFIED,  2,        3 * 1024,     12,     64},
    {   0x49,   CPU_CACHE_UNIFIED,  2,        4 * 1024,     16,     64},    
    {   0x4a,   CPU_CACHE_UNIFIED,  3,        6 * 1024,     12,     64},
    {   0x4b,   CPU_CACHE_UNIFIED,  3,        8 * 1024,     16,     64},
    {   0x4c,   CPU_CACHE_UNIFIED,  3,       12 * 1024,     12,     64},
    {   0x4d,   CPU_CACHE_UNIFIED,  3,       16 * 1024,     16,     64},
    {   0x4e,   CPU_CACHE_UNIFIED,  2,        6 * 1024,     24,     64},
    {   0x60,   CPU_CACHE_DATA,     1,       16,             8,     64},
    {   0x66,   CPU_CACHE_DATA,     1,        8,             4,     64},
    {   0x67,   CPU_CACHE_DATA,     1,       16,             4,     64},
    {   0x68,   CPU_CACHE_DATA,     1,       32,             4,     64},
    {   0x78,   CPU_CACHE_UNIFIED,  2,        1 * 1024,      8,     64},
    {   0x79,   CPU_CACHE_UNIFIED,  2,      128,             8,     64},
    {   0x7a,   CPU_CACHE_UNIFIED,  2,      256,             8,     64},
    {   0x7b,   CPU_CACHE_UNIFIED,  2,      512,             8,     64},
    {   0x7c,   CPU_CACHE_UNIFIED,  2,        1 * 1024,      8,     64},
    {   0x7d,   CPU_CACHE_UNIFIED,  2,        2 * 1024,      8,     64},
    {   0x7f,   CPU_CACHE_UNIFIED,  2,      512,             2,     64},
    {   0x80,   CPU_CACHE_UNIFIED,  2,      512,             8,     64},
    {   0x82,   CPU_CACHE_UNIFIED,  2,      256,             8,     32},
    {   0x83,   CPU_CACHE_UNIFIED,  2,      512,             8,     32},
    {   0x84,   CPU_CACHE_UNIFIED,  2,        1 * 1024,      8,     32},
    {   0x85,   CPU_CACHE_UNIFIED,  2,        2 * 1024,      8,     32},
    {   0x86,   CPU_CACHE_UNIFIED,  2,      512,             4,     64},
    {   0x87,   CPU_CACHE_UNIFIED,  2,        1 * 1024,      8,     64},
    {   0xd0,   CPU_CACHE_UNIFIED,  3,      512,             4,     64},
    {   0xd1,   CPU_CACHE_UNIFIED,  3,        1 * 1024,      4,     64},
    {   0xd2,   CPU_CACHE_UNIFIED,  3,        2 * 1024,      4,     64},
    {   0xd6,   CPU_CACHE_UNIFIED,  3,        1 * 1024,      8,     64},
    {   0xd7,   CPU_CACHE_UNIFIED,  3,        2 * 1024,      8,     64},
    {   0xd8,   CPU_CACHE_UNIFIED,  3,        4 * 1024,      8,     64},
    {   0xdc,   CPU_CACHE_UNIFIED,  3,       (3 * 1024)/2,  12,     64},
    {   0xdd,   CPU_CACHE_UNIFIED,  3,        3 * 1024,     12,     64},
    {   0xde,   CPU_CACHE_UNIFIED,  3,        6 * 1024,     12,     64},
    {   0xe2,   CPU_CACHE_UNIFIED,  3,        4 * 1024,     16,     64},
    {   0xe3,   CPU_CACHE_UNIFIED,  3,        2 * 1024,     16,     64},
    {   0xe4,   CPU_CACHE_UNIFIED,  3,        8 * 1024,     16,     64},
    {   0xea,   CPU_CACHE_UNIFIED,  3,       12 * 1024,     24,     64},
    {   0xeb,   CPU_CACHE_UNIFIED,  3,       18 * 1024,     24,     64},
    {   0xec,   CPU_CACHE_UNIFIED,  3,       24 * 1024,     24,     64},
    
    {   0x00,   0, 0, 0, 0, 0}
};

void cpu_detect_caches(void) {
    unsigned int   idx;
    unsigned int   loop_idx;
    unsigned int   descr;
    unsigned int   size;
    unsigned int   val;
    unsigned int   associativity;
    x86_regs_t     regs;
    
    /* Initialize all cache entries with type CPU_CACHE_NONE. The entry
     * following the last valid one is expected to have this type 
     * (list terminator). */
    for(idx = 0; idx < CPU_CACHE_ENTRIES; ++idx) {
    cpu_caches[idx].type = CPU_CACHE_NONE;
    }
    
    if(cpu_vendor == CPU_VENDOR_INTEL && cpu_cpuid_max >= 2) {
    /** TODO: not yet implemented */
    }
    else if(cpu_vendor == CPU_VENDOR_AMD) {
    idx = 0;
    
    if(cpu_cpuid_ext_max >= 0x80000005) {
        regs.eax = 0x80000005;
        (void)cpuid(&regs);
        
        /* first loop iteration: L1 instruction cache */
        descr = regs.edx;
        
        for(loop_idx = 0; loop_idx < 2; ++loop_idx) {
        size          =  descr >> 24;
        associativity = (descr >> 16) & 0xff;
        if(size > 0 && associativity > 0) {
            cpu_caches[idx].size      = size;
            cpu_caches[idx].level     = 1;
            
            if(loop_idx == 0) {
            cpu_caches[idx].type  = CPU_CACHE_INSTR;
            }
            else {
            cpu_caches[idx].type  = CPU_CACHE_DATA;
            }
            
            cpu_caches[idx].line_size = descr & 0xff;
            
            if(associativity < 255) {
            cpu_caches[idx].associativity = associativity;
            }
            else {
            /* 255 (0xff) means fully associative */
            cpu_caches[idx].associativity = CPU_CACHE_ASSOC_FULL;
            }
            
            ++idx;
        }
        
        /* second loop iteration: L1 instruction cache */
        descr = regs.ecx;
        }
        
    }
    if(cpu_cpuid_ext_max >= 0x80000006) {
        regs.eax = 0x80000006;
        (void)cpuid(&regs);
        
        /* first loop iteration: L2 cache */
        descr = regs.ecx;
        
        for(loop_idx = 0; loop_idx < 2; ++loop_idx) {
        size =  descr >> 16;
        
        if(loop_idx > 0) {
            /* edx (L3 cache) has a different format than ecx (L2).
             * It is stored in bits 31..18 (instead of 31..16) and is in 512kB units.
             * 
             * " [31:18] L3Size: L3 cache size. Specifies the L3 cache size is within the following range:
             *           (L3Size[31:18] * 512KB) <= L3 cache size < ((L3Size[31:18]+1) * 512KB). "
             *  AMD CPUID Specification (Publication # 25481) revision 2.34 pp.25.  */
            size = (size >> 2) * 512;
        }
                
        val  = (descr >> 12) & 0xf;
        
        switch(val) {
        case 1:
        case 2:
        case 4:
            associativity = val;
            break;
        case 0x6:
            associativity = 8;
            break;
        case 0x8:
            associativity = 16;
            break;
        case 0xa:
            associativity = 32;
            break;
        case 0xb:
            associativity = 48;
            break;
        case 0xc:
            associativity = 64;
            break;
        case 0xd:
            associativity = 96;
            break;
        case 0xe:
            associativity = 128;
            break;
        case 0xf:
            associativity = CPU_CACHE_ASSOC_FULL;
            break;
        default:
            associativity = 0;
        }
        
        if(size > 0 && associativity > 0) {
            cpu_caches[idx].size      = size;
            cpu_caches[idx].type      = CPU_CACHE_UNIFIED;
            
            if(loop_idx == 0) {
            cpu_caches[idx].level  = 2;
            }
            else {
            cpu_caches[idx].level  = 3;
            }
            
            cpu_caches[idx].line_size     = descr & 0xff;
            cpu_caches[idx].associativity = associativity;
            
            ++idx;
        }
        
        /* first loop iteration: L3 cache */
        descr = regs.edx;
        }
    }
    }
}
