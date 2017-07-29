#include <hal/asm/boot.h>
#include <hal/asm/vm.h>
#include <hal/asm/x86.h>

    bits 32
    
    ; Used to initialize and identify fields that are set by the setup code. The
    ; actual value does not matter.
    %define MUST_BE_SET_BELOW 0
    
    extern kernel_start
    extern kernel_size
    extern proc_start
    extern proc_size

image_start:
    jmp short header_end

    ; The structure below (i.e. the boot information structure) must match the
    ; boot_info_t struct declaration in include/hal/boot.h.
boot_info_struct:
_kernel_start:      dd kernel_start
_kernel_size:       dd kernel_size
_proc_start:        dd proc_start
_proc_size:         dd proc_size
_image_start:       dd image_start
_image_top:         dd MUST_BE_SET_BELOW
_e820_entries       dd MUST_BE_SET_BELOW
_e820_map           dd MUST_BE_SET_BELOW
_boot_heap          dd MUST_BE_SET_BELOW
_boot_end           dd MUST_BE_SET_BELOW
_setup_signature    dd MUST_BE_SET_BELOW

header_end:
    jmp start

start:
    ; we are going up
    cld
    
    ; On entry, esi points to the real mode code start/zero-page address
    
    ; Copy signature so it can be checked by the kernel
    mov eax, dword [esi + BOOT_SETUP_HEADER]
    mov dword [_setup_signature], eax

    ; figure out the size of the kernel image
    mov eax, dword [esi + BOOT_SYSIZE]
    shl eax, 4              ; times 16
    
    ; align to page boundary
    add eax, PAGE_SIZE - 1
    and eax, ~PAGE_MASK
    
    ; set pointer to top of kernel image
    add eax, BOOT_SETUP32_ADDR
    mov dword [_image_top], eax

    ; --------------------------------------
    ; setup boot-time heap and kernel stack
    ; see doc/layout.txt
    ; --------------------------------------

    ; copy e820 memory map
    mov cl, byte [esi + BOOT_E820_ENTRIES]
    movzx ecx, cl                   ; number of entries
    mov dword [_e820_entries], ecx
    lea ecx, [5 * ecx]              ; times 20 (size of one entry), which is 5 ...
    shl ecx, 2                      ; ... times 2^2
    
    mov edi, eax
    mov dword [_e820_map], edi
    add esi, BOOT_E820_MAP
    
    rep movsb

    ; setup heap: size is four times the size of the bios memory map
    mov eax, edi
    mov [_boot_heap], eax
    
    sub eax, [_image_top]
    shl eax, 2          ; size of heap * 4
    add eax, [_image_top]
    
    ; setup stack and use it
    add eax, BOOT_STACK_SIZE            ; add stack size
    add eax, PAGE_SIZE - 1              ; align address...
    and eax, ~PAGE_MASK                 ; ... on a page boundary
    mov esp, eax                        ; set stack pointer
    
    ; allocate initial page table and page directory
    mov dword [page_table], eax
    add eax, PAGE_SIZE
    mov dword [page_directory], eax
    add eax, PAGE_SIZE
    mov dword [_boot_end], eax
    
    ; initialize initial page table for a 1:1 mapping of the first 4MB
    mov eax, 0 | VM_FLAG_READ_WRITE | VM_FLAG_PRESENT   ; start address is 0, flags
    mov edi, dword [page_table]                         ; write address
    mov ecx, (PAGE_SIZE / 4)                            ; number of entries

init_page_table:
    ; store eax in current page table entry pointed to by edi, then add 4 to edi
    ; to point to the next entry
    stosd
    
    ; update physical address
    add eax, PAGE_SIZE
    
    ; decrement ecx, we are done when it reaches 0, otherwise loop
    loop init_page_table

    ; clear initial page directory
    mov edi, dword [page_directory]     ; write address
    xor eax, eax                        ; write value: 0
    mov cx, PAGE_SIZE                   ; write PAGE_SIZE bytes
    
    rep stosb
    
    ; add entry for the page table that maps the first 4MB
    mov edi, dword [page_directory]
    mov eax, dword [page_table]
    or eax, VM_FLAG_READ_WRITE | VM_FLAG_PRESENT
    mov dword [edi], eax
    
    ; add an alias for the first 4MB of memory at the kernel base address
    mov dword [edi + 4 * (KLIMIT >> 22)], eax
    
    ; set page directory address in CR3
    mov cr3, edi
    
    ; enable paging
    mov eax, cr0
    or eax, X86_CR0_PG
    mov cr0, eax
    
    ; adjust the pointers in the boot information structure so they point in the
    ; kernel alias
    add dword [_kernel_start],  KLIMIT
    add dword [_proc_start],    KLIMIT
    add dword [_image_start],   KLIMIT
    add dword [_image_top],     KLIMIT
    add dword [_e820_map],      KLIMIT
    add dword [_boot_heap],     KLIMIT
    add dword [_boot_end],      KLIMIT
    
    ; adjust stack pointer to point in kernel alias
    add esp, KLIMIT

    ; jump to kernel alias to allow the low address alias to be removed
    jmp just_right_here + KLIMIT
just_right_here:
    
    ; remove the low address alias
    mov eax, dword [page_directory]
    mov dword [eax + KLIMIT], 0         ; clear first page table entry
    
    ; reload CR3 to invalidate all TLB entries for the low address alias
    mov cr3, eax
    
    ; null-terminate call stack (useful for debugging)
    xor eax, eax
    push eax
    push eax
    
    ; initialize frame pointer    
    xor ebp, ebp
    
    ; compute kernel entry point address
    mov esi, kernel_start           ; ELF header
    add esi, KLIMIT
    mov eax, [esi + 24]             ; e_entry member
    
    ; set address of boot information structure in esi for use by the kernel
    mov esi, boot_info_struct
    add esi, KLIMIT                 ; adjust to point in kernel alias
    
    ; jump to kernel entry point
    jmp eax

page_table:     dd 0
page_directory: dd 0
