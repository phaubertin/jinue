#include <hal/e820.h>
#include <printk.h>


e820_t *e820_map;


e820_addr_t e820_get_addr(unsigned int idx) {
    return e820_map[idx].addr;
}

e820_size_t e820_get_size(unsigned int idx) {
    return e820_map[idx].size;
}

e820_type_t e820_get_type(unsigned int idx) {
    return e820_map[idx].type;
}

bool e820_is_valid(unsigned int idx) {
    return (e820_map[idx].size != 0);
}

bool e820_is_available(unsigned int idx) {
    return (e820_map[idx].type == E820_RAM);
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

    for(idx = 0; e820_is_valid(idx); ++idx) {        
        printk("%c %q %q %s\n",
            e820_is_available(idx)?'*':' ',
            e820_get_addr(idx),
            e820_get_size(idx),
            e820_type_description( e820_get_type(idx) )
        );
    }
}
