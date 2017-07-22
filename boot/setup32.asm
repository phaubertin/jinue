#include <hal/asm/boot.h>

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
    ; On entry, esi points to the real mode code start/zero-page address
    
    ; Copy signature so it can be checked by the kernel
    mov eax, dword [esi + BOOT_SETUP_HEADER]
    mov dword [_setup_signature], eax

    ; figure out the size of the kernel image
    mov eax, dword [esi + BOOT_SYSIZE]
    shl eax, 4              ; times 16
    
    ; align to page boundary
    mov ebx, eax
    and ebx, 0xfff
    jz set_image_top        ; already aligned
    add eax, 0x1000         ; page size
    and eax, ~0xfff
    
set_image_top:
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
    
    ; align address on a page boundary
    mov ebx, eax
    and ebx, 0xfff
    jz setup_stack      ; already aligned
    add eax, 0x1000     ; page size
    and eax, ~0xfff
    
    ; setup stack and use it
setup_stack:
    add eax, BOOT_STACK_SIZE
    mov esp, eax
    mov dword [_boot_end], eax
    
    xor eax, eax
    push eax
    push eax        ; null-terminate call stack (useful for debugging)
    xor ebp, ebp    ; initialize frame pointer
    
    ; compute kernel entry point address
    mov esi, kernel_start           ; ELF header
    mov eax, [esi + 24]             ; e_entry member
    
    ; set address of boot information structure in esi for use by the kernel
    mov esi, boot_info_struct
    
    ; jump to kernel entry point
    jmp eax
