#include <jinue/elf.h>
#include <jinue/errno.h>
#include <jinue/pfalloc.h>
#include <jinue/syscall.h>
#include <jinue/types.h>
#include <jinue/vm.h>
#include <printk.h>
#include <stddef.h>
#include <stdint.h>


#define MEMORY_BLOCK_MAX    32


int errno;

Elf32_auxv_t *auxvp;

int main(int argc, char *argv[], char *envp[]) {
    memory_block_t blocks[MEMORY_BLOCK_MAX];
    int method;
    int count;
    uint32_t total_memory;
    unsigned int idx;
    
    /* set system call method and say hello */
    method = set_syscall_method();
    
    printk("Process manager (%s) started.\n", argv[0]);
    printk("Using system call method '%s'.\n", syscall_stub_names[method]);    
    
    /* get free memory blocks from microkernel */
    errno = 0;    
    count = syscall(NULL, NULL, SYSCALL_FUNCT_GET_FREE_MEMORY, (unsigned int)blocks, MEMORY_BLOCK_MAX);
    
    if(errno == JINUE_EMORE) {
        printk("warning: could not get all memory blocks because buffer is too small.\n");
    }
    
    /* count memory */
    total_memory = 0;
    
    for(idx = 0; idx < count; ++idx) {
        total_memory += blocks[idx].count;
    }
    
    printk("%u kilobytes (%u pages) of memory available to process manager.\n", 
        (unsigned long)(total_memory * PAGE_SIZE / KB), 
        (unsigned long)(total_memory) );
    

    char **p;
    
    printk("----- Command line arguments\n");
    printk("argc = %u @0x%x\n", argc, (int)&argc);
    
    for(idx = 0; idx < argc; ++idx) {
        printk("argv[%u] = %s\n", idx, argv[idx]);
    }
    
    printk("\n");
    printk("----- environment variables\n");
    
    idx = 0;
    p = envp;
    
    while(*p != NULL) {
        printk("0x%x envp[%u] = %s\n", (int)p, idx, *p);
        ++p;
        ++idx;
    }
    
    printk("\n");
    printk("----- auxiliary vectors\n");
    if(auxvp == NULL) {
        printk("NULL\n");
    }
    else {
        while(1) {
            char *type;
            
            switch(auxvp->a_type) {
            case AT_NULL: type = "AT_NULL"; break;	
            case AT_IGNORE: type = "AT_IGNORE"; break;
            case AT_EXECFD: type = "AT_EXECFD"; break;
            case AT_PHDR: type = "AT_PHDR"; break;	
            case AT_PHENT: type = "AT_PHENT"; break;
            case AT_PHNUM: type = "AT_PHNUM"; break;
            case AT_PAGESZ: type = "AT_PAGESZ"; break;
            case AT_BASE: type = "AT_BASE"; break;	
            case AT_FLAGS: type = "AT_FLAGS"; break;
            case AT_ENTRY: type = "AT_ENTRY"; break;
            case AT_STACKBASE: type = "AT_STACKBASE"; break;
            case AT_HWCAP: type = "AT_HWCAP"; break;	
            case AT_DCACHEBSIZE: type = "AT_DCACHEBSIZE"; break;
            case AT_ICACHEBSIZE: type = "AT_ICACHEBSIZE"; break;
            case AT_UCACHEBSIZE: type = "AT_UCACHEBSIZE"; break;
            case AT_HWCAP2: type = "AT_HWCAP2"; break;
            case AT_SYSINFO_EHDR: type = "AT_SYSINFO_EHDR"; break;
            default:
                printk("%u => %u\n", auxvp->a_type, auxvp->a_un.a_val);
                type = "???";
            }
            
            printk("0x%x Type %s  0x%x value 0x%x %u\n", (int)auxvp, type, auxvp->a_type, auxvp->a_un.a_val, auxvp->a_un.a_val);
            
            if(auxvp->a_type == AT_NULL) {
                break;
            }
            
            ++auxvp;
        }
    }

    /* loop forever */
    while (1) {}
    
    return 0;
}
