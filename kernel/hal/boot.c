#include <hal/boot.h>
#include <hal/kernel.h>
#include <panic.h>
#include <printk.h>
#include <stddef.h>


const boot_info_t *boot_info;

bool boot_info_check(bool panic_on_failure) {
    /* This data structure is accessed early during the boot process, before
     * paging is enabled. What this means is that, if boot_info is NULL and we
     * dereference it, it does *not* cause a page fault or any other CPU
     * exception. */
    if(boot_info == NULL) {
        if(panic_on_failure) {
            panic("Boot information structure pointer is NULL.");
        }
        
        return false;
    }
    
    if(boot_info->setup_signature != BOOT_SETUP_MAGIC) {
        if(panic_on_failure) {
            panic("Bad setup header signature.");
        }
        
        return false;
    }
    
    return true;
}

const boot_info_t *get_boot_info(void) {
    return boot_info;
}

void boot_info_dump(void) {
    printk("Boot information structure:\n");
    printk("    kernel_start    %x  %u\n", boot_info->kernel_start   , boot_info->kernel_start    );
    printk("    kernel_size     %x  %u\n", boot_info->kernel_size    , boot_info->kernel_size     );
    printk("    proc_start      %x  %u\n", boot_info->proc_start     , boot_info->proc_start      );
    printk("    proc_size       %x  %u\n", boot_info->proc_size      , boot_info->proc_size       );
    printk("    image_start     %x  %u\n", boot_info->image_start    , boot_info->image_start     );
    printk("    image_top       %x  %u\n", boot_info->image_top      , boot_info->image_top       );
    printk("    e820_entries    %x  %u\n", boot_info->e820_entries   , boot_info->e820_entries    );
    printk("    e820_map        %x  %u\n", boot_info->e820_map       , boot_info->e820_map        );
    printk("    boot_heap       %x  %u\n", boot_info->boot_heap      , boot_info->boot_heap       );
    printk("    boot_end        %x  %u\n", boot_info->boot_end       , boot_info->boot_end        );
    printk("    page_table      %x  %u\n", boot_info->page_table     , boot_info->page_table      );
    printk("    page_directory  %x  %u\n", boot_info->page_directory , boot_info->page_directory  );
    printk("    setup_signature %x  %u\n", boot_info->setup_signature, boot_info->setup_signature );
}
