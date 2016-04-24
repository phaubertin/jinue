#include <hal/kernel.h>
#include <hal/thread.h>
#include <hal/vm.h>
#include <assert.h>
#include <elf.h>
#include <panic.h>
#include <pfalloc.h>
#include <printk.h>


void elf_check(Elf32_Ehdr *elf) {
    /* check: valid ELF binary magic number */
    if(		elf->e_ident[EI_MAG0] != ELF_MAGIC0 ||
    		elf->e_ident[EI_MAG1] != ELF_MAGIC1 ||
			elf->e_ident[EI_MAG2] != ELF_MAGIC2 ||
			elf->e_ident[EI_MAG3] != ELF_MAGIC3 ) {
        panic("Not an ELF binary");
    }
    
    /* check: 32-bit objects */
    if(elf->e_ident[EI_CLASS] != ELFCLASS32) {
        panic("Bad file class");
    }
    
    /* check: endianess */
    if(elf->e_ident[EI_DATA] != ELFDATA2LSB) {
        panic("Bad endianess");
    }
    
    /* check: version */
    if(elf->e_version != 1 || elf->e_ident[EI_VERSION] != 1) {
        panic("Not ELF version 1");
    }
    
    /* check: machine */
    if(elf->e_machine != EM_386) {
        panic("This process manager binary does not target the x86 architecture");
    }
    
    /* check: the 32-bit Intel architecture defines no flags */
    if(elf->e_flags != 0) {
        panic("Invalid flags specified");
    }
    
    /* check: file type is executable */
    if(elf->e_type != ET_EXEC) {
        panic("process manager binary is not an an executable");
    }
    
    /* check: must have a program header */
    if(elf->e_phoff == 0 || elf->e_phnum == 0) {
        panic("No program headers");
    }
    
    /* check: must have an entry point */
    if(elf->e_entry == 0) {
        panic("No entry point for process manager");
    }
    
    /* check: program header entry size */
    if(elf->e_phentsize != sizeof(Elf32_Phdr)) {
        panic("Unsupported program header size");
    }
}

void elf_load(elf_info_t *info, Elf32_Ehdr *elf, addr_space_t *addr_space) {
    Elf32_Phdr *phdr;
    pfaddr_t page;
    addr_t vpage;
    char *vptr, *vend, *vfend, *vnext;
    char *file_ptr;    
    char *stop;
    unsigned int idx;
    unsigned long flags;
    
    
    /* check that ELF binary is valid */
    elf_check(elf);
    
    /* get the program header table */
    phdr = (Elf32_Phdr *)((char *)elf + elf->e_phoff);
    
    info->at_phdr       = (addr_t)phdr;
    info->at_phnum      = elf->e_phnum;
    info->at_phent      = elf->e_phentsize;
    info->addr_space    = addr_space;
    info->entry         = (addr_t)elf->e_entry;
    
    /* load the ELF binary*/
    for(idx = 0; idx < elf->e_phnum; ++idx) {
        if(phdr[idx].p_type != PT_LOAD) {
            continue;
        }
        
        /* check that the segment is not in the region reserved for kernel use */
        if(phdr[idx].p_vaddr < (Elf32_Addr)PLIMIT) {
            panic("process manager memory layout -- address of segment too low");
        }
        
        /* set start and end addresses for mapping and copying */
        file_ptr = (char *)elf + phdr[idx].p_offset;
        vptr     = (char *)phdr[idx].p_vaddr;
        vend     = vptr  + phdr[idx].p_memsz;  /* limit for padding */
        vfend    = vptr  + phdr[idx].p_filesz; /* limit for copy */
                
        /* align on page boundaries, be inclusive, 
           note that vfend is not aligned        */
        file_ptr = (char *) ( (uintptr_t)file_ptr & ~PAGE_MASK );
        vptr     = (char *) ( (uintptr_t)vptr  & ~PAGE_MASK );
        
        if(page_offset_of(vend) != 0) {
            vend  = (char *) ( (uintptr_t)vend  & ~PAGE_MASK );
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
                vm_map(addr_space, vpage, page, VM_FLAG_KERNEL | VM_FLAG_READ_WRITE);
                
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
                vm_change_flags(addr_space, vpage, flags);                
            }
        }
        else {            
            while(vptr < vend) {
                /** TODO: add exec flag once PAE is enabled */
                /* set flags */
                flags = VM_FLAG_USER | VM_FLAG_READ_ONLY;
            
                /* perform mapping */
                vm_map(addr_space, (addr_t)vptr, PTR_TO_PFADDR(file_ptr), flags);
                
                vptr     += PAGE_SIZE;
                file_ptr += PAGE_SIZE;
            }            
        }
    }
    
    elf_setup_stack(info);
    
    printk("ELF binary loaded.\n");
}

void elf_setup_stack(elf_info_t *info) {
    pfaddr_t page;
    addr_t vpage;
    
    /** TODO: check for overlap of stack with loaded segments */
    
    /* initial stack allocation */
    for(vpage = (addr_t)STACK_START; vpage < (addr_t)STACK_BASE; vpage += PAGE_SIZE) {        
        page  = pfalloc();
        vm_map(info->addr_space, vpage, page, VM_FLAG_USER | VM_FLAG_READ_WRITE);
    }
    
    uint32_t *sp = (uint32_t *)STACK_BASE;
    
    /* Program name string: "proc", null-terminated */
    *(--sp) = 0;
    *(--sp) = 0x636f7270;
    
    char *argv0 = (char *)sp;
    
    /* auxiliary vectors */
    Elf32_auxv_t *auxvp = (Elf32_auxv_t *)sp - 7;
    
    auxvp[0].a_type     = AT_PHDR;
    auxvp[0].a_un.a_val = (int32_t)info->at_phdr;
    
    auxvp[1].a_type     = AT_PHENT;
    auxvp[1].a_un.a_val = (int32_t)info->at_phent;
    
    auxvp[2].a_type     = AT_PHNUM;
    auxvp[2].a_un.a_val = (int32_t)info->at_phnum;
    
    auxvp[3].a_type     = AT_PAGESZ;
    auxvp[3].a_un.a_val = PAGE_SIZE;
    
    auxvp[4].a_type     = AT_ENTRY;
    auxvp[4].a_un.a_val = (int32_t)info->entry;
    
    auxvp[5].a_type     = AT_STACKBASE;
    auxvp[5].a_un.a_val = STACK_BASE;
    
    auxvp[6].a_type     = AT_NULL;
    auxvp[6].a_un.a_val = 0;
    
    sp = (uint32_t *)auxvp;
    
    /* empty environment variables */
    *(--sp) = 0;
    
    /* argv with only program name */
    *(--sp) = 0;
    *(--sp) = (uint32_t)argv0;
    
    /* argc */
    *(--sp) = 1;
    
    info->stack_addr = (addr_t)sp;
}

void elf_start(elf_info_t *info) {
    /* leaving the kernel */
    in_kernel = 0;
    
    /* this never returns */
    thread_start(info->entry, info->stack_addr);
    
    panic("thread_start() returned in elf_start()");
}
