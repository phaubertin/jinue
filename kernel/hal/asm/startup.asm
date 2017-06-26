%define BOOT_STACK_SIZE 8192

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
    
    ; point esi and eax to the start of boot sector data
    ; (setup code pushed its start address on stack)
    pop eax
    sub eax, 19
    mov esi, eax
    
    ; figure out size of kernel image
    add eax, 7              ; field: sysize
    mov eax, dword [eax]
    shl eax, 4
;    mov dword [kernel_size], eax
    
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
    rep movsb
    mov eax, edi
    
    ; copy e820 map data
    mov dword [e820_map], eax
    mov edi, eax
    mov esi, 0x7c00

e820_loop:
    mov eax, edi
    
    mov ecx, 20     ; size of one entry
    rep movsb
    
    add eax, 8      ; size field, first word
    mov ecx, dword [eax]
    or  ecx, ecx
    jnz e820_loop
    
    add eax, 4      ; size field, second word
    mov ecx, dword [eax]
    or  ecx, ecx
    jz e820_end
    
    jmp e820_loop    
e820_end:

    ; setup heap, size is four times the size of the bios memory map
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
