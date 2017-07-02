#include <hal/asm/startup.h>
#include <hal/asm/boot.h>

bits 32

global halt
global __start

extern kmain
extern e820_map
extern boot_data
extern boot_heap
extern kernel_size
extern kernel_top
extern kernel_region_top
extern kernel_end

__start:
    ; set kernel size
    mov eax, kernel_end
    sub eax, __start
    mov dword [kernel_size], eax
    
    ; On entry, esi points to the real mode code start/zero-page address

    ; figure out the size of the kernel image
    mov eax, dword [esi + BOOT_SYSIZE]
    shl eax, 4
    
    ; align to page boundary
    mov ebx, eax
    and ebx, 0xfff
    jz set_kernel_top       ; already aligned
    add eax, 0x1000         ; page size
    and eax, ~0xfff
    
set_kernel_top:
    add eax, __start
    mov dword [kernel_top], eax
    
    ; --------------------------------------
    ; setup boot-time heap and kernel stack
    ; see doc/layout.txt
    ; --------------------------------------
    
    ; copy boot sector data
    mov edi, eax
    mov dword [boot_data], eax
    mov ecx, 32
    
    push esi
    
    add esi, BOOT_DATA_STRUCT
    rep movsb
    
    ; Restore esi
    ;
    ; edi still points to the first byte after the copied data.
    pop esi
    
    ; copy e820 map data
    mov dword [e820_map], edi
    
    mov cl, byte [esi + BOOT_E820_ENTRIES]
    movzx ecx, cl               ; number of entries
    lea ecx, [5 * ecx]          ; times 20 (size of one entry), which is 5 ...
    shl ecx, 2                  ; ... times 2^2
    
    add esi, BOOT_E820_MAP
    
    rep movsb
    
    ; add a zero-length entry to indicate the end of the map
    mov ecx, 20
    xor eax, eax
    rep stosb
    
    ; setup heap: size is four times the size of the bios memory map
    mov eax, edi
    mov [boot_heap], eax
    
    sub eax, [kernel_top]
    shl eax, 2          ; size of heap * 4
    add eax, [kernel_top]
    
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
    mov dword [kernel_region_top], eax
    
    xor eax, eax
    push eax
    push eax        ; null-terminate call stack (useful for debugging)
    xor ebp, ebp    ; initialize frame pointer
    
start_kernel:
    call kmain

halt:
    cli
    hlt
    jmp halt
