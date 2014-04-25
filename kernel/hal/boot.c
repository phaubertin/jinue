#include <boot.h>
#include <kernel.h>
#include <panic.h>


addr_t  boot_data;


boot_t *get_boot_data(void) {
    boot_t *boot;
    
    boot = (boot_t *)boot_data;
        
    if(boot->signature != BOOT_SIGNATURE) {
        panic("bad boot sector signature.");
    }
    
    if(boot->magic != BOOT_MAGIC) {
        panic("bad boot sector magic.");
    }
    
    return boot;
}
