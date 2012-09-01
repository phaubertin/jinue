#include <hal/thread.h>
#include <hal/vm.h>
#include <assert.h>
#include <elf.h>
#include <panic.h>
#include <pfalloc.h>
#include <printk.h>


void elf_check_process_manager(void) {
	/* check that Process manager binary is valid */
	if( *(unsigned long *)&proc_elf != ELF_MAGIC ) {
		panic("Process manager not found");
	}
		
	/* CHECK 1: file size */
	if((unsigned long)&proc_elf_end < (unsigned long)&proc_elf) {
		panic("Malformed boot image");
	}
	
	if((unsigned int)&proc_elf_end - (unsigned int)&proc_elf < sizeof(elf_header_t)) {
		panic("Too small to be an ELF binary");
	}
	
	printk("Found process manager binary with size %u bytes.\n", (unsigned int)&proc_elf_end - (unsigned int)&proc_elf);
	
	/* CHECK 2: 32-bit objects */
	if(proc_elf.e_ident[EI_CLASS] != ELFCLASS32) {
		panic("Bad file class");
	}
	
	/* CHECK 3: endianess */
	if(proc_elf.e_ident[EI_DATA] != ELFDATA2LSB) {
		panic("Bad endianess");
	}
	
	/* CHECK 4: version */
	if(proc_elf.e_version != 1 || proc_elf.e_ident[EI_VERSION] != 1) {
		panic("Not ELF version 1");
	}
	
	/* CHECK 5: machine */
	if(proc_elf.e_machine != EM_386) {
		panic("This process manager binary does not target the x86 architecture");
	}
	
	/* CHECK 6: the 32-bit Intel architecture defines no flags */
	if(proc_elf.e_flags != 0) {
		panic("Invalid flags specified");
	}
	
	/* CHECK 7: file type is executable */
	if(proc_elf.e_type != ET_EXEC) {
		panic("process manager binary is not an an executable");
	}
	
	/* CHECK 8: must have a program header */
	if(proc_elf.e_phoff == 0 || proc_elf.e_phnum == 0) {
		panic("No program headers");
	}
	
	/* CHECK 9: must have an entry point */
	if(proc_elf.e_entry == 0) {
		panic("No entry point for process manager");
	}
	
	/** TODO: we can do better than this */
	/* CHECK 10: program header entry size */
	if(proc_elf.e_phentsize != sizeof(elf_prog_header_t)) {
		panic("Unsupported program header size");
	}
}

void elf_load_process_manager(void) {
	elf_prog_header_t *phdr;
	pfaddr_t page;
	addr_t vpage;
	char *vptr, *vend, *vfend, *vnext;
	char *file_ptr;	
	char *stop;
	unsigned long idx;
	unsigned long flags;
	
	
	/* check that Process manager binary is valid */
	elf_check_process_manager();
	
	/** This is what we do:
	    - We are only interested in loadable segments (PT_LOAD).
	   
	    - It is an error if dynamic linking information (PT_DYNAMIC) is
	      present, or if a program interpreter is requested (PT_INTERP).
	  
	    - All other segment types are ignored
	  
	    - For writable loadable segments and for loadable segments of
	      which the memory size is greater than the file size, the
	      content is copied in newly allocated memory pages, and then
	      mapped.
	  
	    - For other loadable segments (ie. read-only and for which
	      memory size and file size are the same), the segment is
	      directly mapped at its proper address without copying. 	 
	 */
	
	/* get the program header table */
	phdr = (elf_prog_header_t *)((void *)&proc_elf + proc_elf.e_phoff); 
	
	/* look for PT_DYNAMIC and PT_INTERP segments */
	for(idx = 0; idx < proc_elf.e_phnum; ++idx) {
		if(phdr[idx].p_type == PT_DYNAMIC) {
			panic("Process manager binary contains dynamic linking information");
		}
		if(phdr[idx].p_type == PT_INTERP) {
			panic("Program interpreter requested for process manager binary");
		}
	}
	
	/* load the process manager */
	for(idx = 0; idx < proc_elf.e_phnum; ++idx) {
		if(phdr[idx].p_type != PT_LOAD) {
			continue;
		}
		
		/* check that the segment is not in the region reserved for kernel use */
		if(phdr[idx].p_vaddr < (addr_t)PLIMIT) {
			panic("process manager memory layout -- address of segment too low");
		}
		
		/* set start and end addresses for mapping and copying */
		file_ptr = (char *)&proc_elf + phdr[idx].p_offset;
		vptr     = (char *)phdr[idx].p_vaddr;
		vend     = vptr  + phdr[idx].p_memsz;  /* limit for padding */
		vfend    = vptr  + phdr[idx].p_filesz; /* limit for copy */
				
		/* align on page boundaries, be inclusive, 
		   note that vfend is not aligned        */
		file_ptr = (char *) ( (unsigned long)file_ptr & ~PAGE_MASK );
		vptr     = (char *) ( (unsigned long)vptr  & ~PAGE_MASK );
		
		if(PAGE_OFFSET_OF(vend) != 0) {
			vend  = (char *) ( (unsigned long)vend  & ~PAGE_MASK );
			vend +=  PAGE_SIZE;
		}		
	
		/* copy if we have to */
		if( (phdr[idx].p_flags & PF_W) || (phdr[idx].p_filesz != phdr[idx].p_memsz) ) {
			while(vptr < vend) {
				/** TODO: add exec flag once PAE is enabled */
				/* set flags */
				flags = VM_FLAG_USER;
				
				if(phdr[idx].p_flags & PF_W) {
					flags |= VM_FLAG_READ_WRITE;
				}
				else  {
					flags |= VM_FLAG_READ_ONLY;
				}
				
				/* start of this page and next page */
				vpage = (addr_t)vptr;
				vnext = vptr + PAGE_SIZE;
				
				/* allocate and map the new page */
				page = pfalloc();
				vm_map(vpage, page, VM_FLAG_KERNEL | VM_FLAG_READ_WRITE);
				
				/* copy */
				stop     = vnext;
				if(stop > vfend) {
					stop = vfend;
				}
				
				while(vptr < stop) {
					*(vptr++) = *(file_ptr++);
				}
				
				/* pad */
				while(vptr < vnext) {
					*(vptr++) = 0;
				}
				
				/* set proper flags for page now that we no longer need to write in it */
				vm_change_flags(vpage, flags);				
			}
		}
		else {			
			while(vptr < vend) {
				/** TODO: add exec flag once PAE is enabled */
				/* set flags */
				flags = VM_FLAG_USER | VM_FLAG_READ_ONLY;
			
				/* perform mapping */
				vm_map((addr_t)vptr, PTR_TO_PFADDR(file_ptr), flags);
				
				vptr     += PAGE_SIZE;
				file_ptr += PAGE_SIZE;
			}			
		}
	}
	
	/** TODO: we should probably do better than this, at least check for overlap */
	/* setup stack */
	page  = pfalloc();
	vpage = (addr_t)0xfffff000;
	vm_map(vpage, page, VM_FLAG_USER | VM_FLAG_READ_WRITE);
	
	printk("Process manager loaded.\n");
}

void elf_start_process_manager(void) {
	elf_entry_t entry;
	
	entry = (elf_entry_t)proc_elf.e_entry;
	
	/* this never returns */
	thread_start((addr_t)entry, (addr_t)0xfffffff0);
	
	panic("Process Manager returned");
}
