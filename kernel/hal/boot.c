#include <hal/boot.h>
#include <hal/kernel.h>
#include <panic.h>
#include <stddef.h>


addr_t  boot_data;


boot_t *get_boot_data(void) {
    boot_t *boot;
    
    boot = (boot_t *)boot_data;
    
    if(boot == NULL) {
        panic("Boot data structure pointer is NULL.");
    }
        
    if(boot->signature != BOOT_SIGNATURE) {
        panic("Bad boot sector signature");
    }
    
    return boot;
}
