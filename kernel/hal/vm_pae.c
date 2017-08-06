#include <hal/vm_private.h>
#include <hal/boot.h>
#include <hal/bootmem.h>
#include <hal/x86.h>
#include <assert.h>
#include <panic.h>
#include <printk.h>
#include <pfalloc.h>
#include <slab.h>
#include <string.h>
#include <util.h>
#include <vm_alloc.h>


/** number of address bits that encode the PDPT offset */
#define PDPT_BITS               2

/** number of entries in a Page Directory Pointer Table (PDPT) */
#define PDPT_ENTRIES            (1 << PDPT_BITS)


struct pte_t {
    uint64_t entry;
};

struct pdpt_t {
    pte_t   pd[PDPT_ENTRIES];
};

/** slab cache that allocates Page Directory Pointer Tables (PDPTs) */
static slab_cache_t *pdpt_cache;

pdpt_t *initial_pdpt;

/** Get the Page Directory Pointer Table (PDPT) index of a virtual address
 *  @param addr virtual address
 * */
static inline unsigned int pdpt_offset_of(void *addr) {
    return (uintptr_t)addr >> (32 - PDPT_BITS);
}

/** 
    Lookup and map the page directory for a specified address and address space.
    
    Important note: it is the caller's responsibility to unmap and free the returned
    page directory when it is done with it.
    
    @param addr_space address space in which the address is looked up.
    @param addr address to look up
    @param create_as_need Whether a page table is allocated if it does not exist
*/
static pte_t *vm_pae_lookup_page_directory(addr_space_t *addr_space, void *addr, bool create_as_needed) {
    pdpt_t *pdpt    = addr_space->top_level.pdpt;
    pte_t  *pdpte   = &pdpt->pd[pdpt_offset_of(addr)];
    
    if(get_pte_flags(pdpte) & VM_FLAG_PRESENT) {
        /* map page directory */
        pte_t *page_directory   = (pte_t *)vm_alloc(global_page_allocator);
        vm_map_kernel((addr_t)page_directory, get_pte_pfaddr(pdpte), VM_FLAG_READ_WRITE);
        
        return page_directory;
    }
    else {
        if(create_as_needed) {
            /* allocate a new page directory and map it */
            pte_t *page_directory   = (pte_t *)vm_alloc(global_page_allocator);
            pfaddr_t pf_pd          = pfalloc();
        
            vm_map_kernel((addr_t)page_directory, pf_pd, VM_FLAG_READ_WRITE);
            
            /* zero content of page directory */
            memset(page_directory, 0, PAGE_SIZE);
            
            /* link page directory in PDPT */
            set_pte(pdpte, pf_pd, VM_FLAG_PRESENT);
            
            return page_directory;
        }
        else {
            return NULL;
        }
    }
}

static unsigned int vm_pae_page_table_offset_of(addr_t addr) {
    return PAGE_TABLE_OFFSET_OF(addr);
}

static unsigned int vm_pae_page_directory_offset_of(addr_t addr) {
    return PAGE_DIRECTORY_OFFSET_OF(addr);
}

static pte_t *vm_pae_get_pte_with_offset(pte_t *pte, unsigned int offset) {
    return &pte[offset];
}

/** TODO handle flag bit position > 31 for NX bit support */
static void vm_pae_set_pte(pte_t *pte, pfaddr_t paddr, int flags) {
    pte->entry = ((uint64_t)paddr << PFADDR_SHIFT) | flags;
}

/** TODO handle flag bit position > 31 for NX bit support */
static void vm_pae_set_pte_flags(pte_t *pte, int flags) {
    pte->entry = (pte->entry & ~(uint64_t)PAGE_MASK) | flags;
}

static int vm_pae_get_pte_flags(pte_t *pte) {
    return pte->entry & PAGE_MASK;
}

static pfaddr_t vm_pae_get_pte_pfaddr(pte_t *pte) {
    return (pte->entry & ~(uint64_t)PAGE_MASK) >> PFADDR_SHIFT;
}

static void vm_pae_clear_pte(pte_t *pte) {
    pte->entry = 0;
}

static void vm_pae_copy_pte(pte_t *dest, pte_t *src) {
    dest->entry = src->entry;
}

void vm_pae_enable(void) {
    uint32_t temp = get_cr4();
    set_cr4(temp | X86_CR4_PAE);
}

void vm_pae_create_pdpt_cache(void) {
    pdpt_cache = slab_cache_create(
            "vm_pae_pdpt_cache",
            sizeof(pdpt_t),
            sizeof(pdpt_t),
            NULL,
            NULL,
            SLAB_DEFAULTS);
            
    if(pdpt_cache == NULL) {
        panic("Cannot create Page Directory Pointer Table (PDPT) slab cache.");
    }
}

/** Return whether the page directory at the specified PDPT index is split between
 *  user space and kernel space
 * 
 * @param pdpt_index Page Directory Pointer Table (PDPT) index of page directory */
static inline bool is_split_page_directory(unsigned int pdpt_index) {
    return pdpt_index == pdpt_offset_of((addr_t)KLIMIT)
            /* If KLIMIT is the first entry of this page directory, then the page
             * directory describes only kernel space. */
            && page_directory_offset_of((addr_t)KLIMIT) > 0;
}

static addr_space_t *vm_pae_create_addr_space(addr_space_t *addr_space) {
    unsigned int idx;
    pte_t *pdpte;
    
    /* Use the initial address space as a template for the kernel address range
     * (address KLIMIT and above). The page tables for that range are shared by
     * all address spaces. */
    pdpt_t *template_pdpt = initial_addr_space.top_level.pdpt;
    
    /* Create a PDPT for the new address space */
    pdpt_t *pdpt = slab_cache_alloc(pdpt_cache);
    
    for(idx = 0; idx < PDPT_ENTRIES; ++idx) {
        pdpte = &pdpt->pd[idx];
        
        if(idx < pdpt_offset_of((addr_t)KLIMIT)) {
            /* This PDPT entry describes an address range entirely under KLIMIT
             * so it is all user space: do not create a page directory at this
             * time. */
             clear_pte(pdpte);
        }
        else if(is_split_page_directory(idx)) {
            /* KLIMIT splits the address range described by this PDPT entry
             * between user space and kernel space: create a page directory and
             * clone entries for KLIMIT and above from the template. */
            pfaddr_t pfaddr = vm_clone_page_directory(
                    get_pte_pfaddr(&template_pdpt->pd[idx]),
                    page_directory_offset_of((addr_t)KLIMIT));
            
            set_pte(pdpte, pfaddr, VM_FLAG_PRESENT);
        }
        else {
            /* This page directory describes an address range entirely above
             * KLIMIT: share the template's page directory. */
            copy_pte(pdpte, &template_pdpt->pd[idx]);
        }
    }
    
    /* Lookup the physical address of the page where the PDPT resides. */
    pfaddr_t pdpt_pfaddr = vm_lookup_pfaddr(NULL, (addr_t)page_address_of(pdpt));
    
    /** ASSERTION: Page Directory Pointer Table (PDPT) must be in the first 4GB of memory */
    assert( PFADDR_CHECK_4GB(pdpt_pfaddr) );
    
    /* physical address of PDPT */
    uint32_t pdpt_phys_addr = (uint32_t)PFADDR_TO_ADDR(pdpt_pfaddr) | page_offset_of(pdpt);
    
    addr_space->top_level.pdpt  = pdpt;
    addr_space->cr3             = pdpt_phys_addr;
    
    return addr_space;
}

static addr_space_t *vm_pae_create_initial_addr_space(void) {
    unsigned int idx;
    pte_t *pdpte;
    
    /* The 32-bit setup code (see boot/setup32.asm) enables paging before
     * passing control to the kernel. This is done to allow the kernel to run
     * at an address that is different from the one at which it is loaded, but
     * it creates bootstrapping issues when enabling PAE.
     * 
     * The problem we are trying to get around is the following: if we enable
     * PAE before setting the address of the PAE paging structures in CR3, the
     * CPU will try to load the PDPT registers from old address in CR3, which is
     * the address of the page directory set up by the 32-bit setup code. If,
     * on the other hand, we load the address of the PAE paging structures in
     * CR3 before enabling PAE, the CPU will try to use the PDPT as a page
     * directory for standard 32-bit (i.e. non-PAE) address translation.
     * 
     * The solution that is implemented here is to copy the initial PDPT at the
     * start of the page directory set up by the 32-bit setup code. By doing
     * this, the address of that page directory becomes valid both as a non-PAE
     * page directory and as a PDPT, which allows us to enable PAE before loading
     * a new address in CR3. Of course, for this to work, the kernel must not
     * access memory under 0x2000000 (8 page directory entries * 4MB/pde = 32MB).
     * 
     * We copy the intial PDPT to the page directory set up by the 32-bit setup
     * code instead of simply allocating it there permanently because we want to
     * be able to reclaim that page after CR3 has been reloaded. */
    
    /* Allocate intial PDPT
     * 
     * PDPT must be 32-byte aligned */
    initial_pdpt    = (pdpt_t *)ALIGN_END(boot_heap, 32);
    boot_heap       = initial_pdpt + 1;
    
    /* The copy is at the start of the 32-bit setup code-allocated page directory */
    const boot_info_t *boot_info = get_boot_info();
    pdpt_t *pdpt_copy = (pdpt_t *)boot_info->page_directory;
    
    for(idx = 0; idx < PDPT_ENTRIES; ++idx) {
        pdpte = &initial_pdpt->pd[idx];
        
        if(idx < pdpt_offset_of((addr_t)KLIMIT)) {
            /* This PDPT entry describes an address range entirely under KLIMIT
             * so it is all user space: do not create a page directory at this
             * time. */
             clear_pte(pdpte);
        }
        else if(idx == pdpt_offset_of((addr_t)KLIMIT)) {
            /* Allocate the first page directory and allocate page tables starting
             * at KLIMIT. Specify that this is the first page directory. */
            pte_t *page_directory = vm_allocate_page_directory(
                    page_directory_offset_of((addr_t)KLIMIT), true);
            
            set_pte(pdpte, EARLY_PTR_TO_PFADDR(page_directory), VM_FLAG_PRESENT);
        }
        else {
            /* allocate page directory and allocate all its page tables */
            pte_t *page_directory = vm_allocate_page_directory(
                    page_table_entries, false);
            
            set_pte(pdpte, EARLY_PTR_TO_PFADDR(page_directory), VM_FLAG_PRESENT);
        }
        
        /* copy entry to old page directory */
        vm_pae_copy_pte(&pdpt_copy->pd[idx], pdpte);
    }
    
    initial_addr_space.top_level.pdpt   = initial_pdpt;
    initial_addr_space.cr3              = EARLY_VIRT_TO_PHYS(initial_pdpt);
    
    return &initial_addr_space;
}

static void vm_pae_destroy_addr_space(addr_space_t *addr_space) {
    unsigned int idx;
    pte_t pdpte;
    
    pdpt_t *pdpt = addr_space->top_level.pdpt;
    
    for(idx = 0; idx < PDPT_ENTRIES; ++idx) {
        pdpte.entry = pdpt->pd[idx].entry;
        
        if(idx < pdpt_offset_of((addr_t)KLIMIT)) {
            /* This page directory describes an address range entirely under
             * KLIMIT so it is all user space: free all page tables in this
             * page directory as well as the page directory itself. */
            if(pdpte.entry & VM_FLAG_PRESENT) {
                vm_destroy_page_directory(
                        get_pte_pfaddr(&pdpte),
                        0,
                        page_table_entries);
            }
        }
        else if(is_split_page_directory(idx)) {
            /* KLIMIT splits the address range described by this PDPT entry
             * between user space and kernel space: free only the user space page
             * tables (i.e. for the address range up to KLIMIT), then free the
             * page directory.
             * 
             * The page tables for KLIMIT and above must not be freed because
             * they describe the kernel address range and are shared by all
             * address spaces. */
            if(pdpte.entry & VM_FLAG_PRESENT) {
                vm_destroy_page_directory(
                        get_pte_pfaddr(&pdpte),
                        0,
                        page_directory_offset_of((addr_t)KLIMIT));
            }
        }
        else {
            /* This page directory describes an address range entirely above
             * KLIMIT: do nothing.
             * 
             * The page directory must not be freed because it is shared by all
             * address spaces. */
        }
    }
    
    slab_cache_free(pdpt);
}

void vm_pae_boot_init(void) {
    page_table_entries          = (size_t)PAGE_TABLE_ENTRIES;
    create_addr_space           = vm_pae_create_addr_space;
    create_initial_addr_space   = vm_pae_create_initial_addr_space;
    destroy_addr_space          = vm_pae_destroy_addr_space;
    page_table_offset_of        = vm_pae_page_table_offset_of;
    page_directory_offset_of    = vm_pae_page_directory_offset_of;
    lookup_page_directory       = vm_pae_lookup_page_directory;
    get_pte_with_offset         = vm_pae_get_pte_with_offset;
    set_pte                     = vm_pae_set_pte;
    set_pte_flags               = vm_pae_set_pte_flags;
    get_pte_flags               = vm_pae_get_pte_flags;
    get_pte_pfaddr              = vm_pae_get_pte_pfaddr;
    clear_pte                   = vm_pae_clear_pte;
    copy_pte                    = vm_pae_copy_pte;
}
