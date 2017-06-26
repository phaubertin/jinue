#include <jinue/page_tables.h>
#include <hal/vm_macros.h>
#include <hal/vm_x86.h>
#include <hal/vm.h>
#include <hal/x86.h>
#include <assert.h>
#include <stdint.h>
#include <pfalloc.h>


struct __pte_t {
    uint32_t entry;
};


static unsigned int vm_x86_page_table_offset_of(addr_t addr) {
    return PAGE_TABLE_OFFSET_OF(addr);
}

static unsigned int vm_x86_page_directory_offset_of(addr_t addr) {
    return PAGE_DIRECTORY_OFFSET_OF(addr);
}

void vm_x86_set_pointers(void) {
    vm_x86_set_pte_pointers();
    
    page_table_entries          = (size_t)PAGE_TABLE_ENTRIES;
    
    page_table_offset_of        = vm_x86_page_table_offset_of;
    page_directory_offset_of    = vm_x86_page_directory_offset_of;
}
