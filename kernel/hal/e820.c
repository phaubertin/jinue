#include <hal/boot.h>
#include <hal/e820.h>
#include <printk.h>

bool e820_is_valid(const e820_t *e820_entry) {
    return e820_entry->size != 0;
}

bool e820_is_available(const e820_t *e820_entry) {
    return e820_entry->type == E820_RAM;
}

const char *e820_type_description(e820_type_t type) {
    switch(type) {
    
    case E820_RAM:
        return "available";
    
    case E820_RESERVED:
        return "unavailable/reserved";
    
    case E820_ACPI:
        return "unavailable/acpi";
    
    default:
        return "unavailable/other";
    }    
}

void e820_dump(void) {
    unsigned int idx;
    
    printk("Dump of the BIOS memory map:\n");
    printk("  address  size     type\n");

    const boot_info_t *boot_info = get_boot_info();

    for(idx = 0; idx < boot_info->e820_entries; ++idx) {
        const e820_t *e820_entry = &boot_info->e820_map[idx];

        printk("%c %q %q %s\n",
            e820_is_available(e820_entry)?'*':' ',
            e820_entry->addr,
            e820_entry->size,
            e820_type_description(e820_entry->type)
        );
    }
}
