#include <hal/asm/boot.h>
#include <hal/asm/e820.h>

%define CODE_SEG        1
%define DATA_SEG        2
%define SETUP_SEG       3

    bits 16
    jmp short start

header:             db "HdrS"
version:            dw 0x0204
realmode_swtch:     dd 0
start_sys:          dw 0x1000
kernel_version:     dw str_version
type_of_loader:     db 0
loadflags:          db 1
setup_move_size:    dw 0
code32_start:       dd BOOT_SETUP32_ADDR
ramdisk_image:      dd 0
ramdisk_size:       dd 0
bootsect_kludge:    dd 0
heap_end_ptr:       dw 0
pad1:               dw 0
cmd_line_ptr:       dd 0
initrd_addr_max:
ramdisk_max:        dd 0

start:
    ; Setup the segment registers
    mov ax, cs
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Adjust the stack pointer to point to the same address as before in the new
    ; segment
    sub sp, 0x200
    
    ; Compute the setup code start address
    movzx eax, ax
    shl eax, 4
    
    ; Push the real mode code start address (setup code start address - 0x200)
    ; on the stack for later
    mov ebx, eax
    sub ebx, 0x200    
    push ebx
    
    ; Determine the address of the GDT    
    mov ebx, eax
    add ebx, gdt
    mov [gdt_info.addr], ebx
    
    ; Patch the GDT
    or eax, dword [gdt.setup+2]
    mov [gdt.setup+2], eax
    
    ; Activate the A20 gate
    call enable_a20
    
    ; Get memory map
    call bios_e820
    
    ; Load GDT and IDT registers
    lgdt [gdt_info]
    lidt [gdt.null+2]   ; A bunch of zeros, properly aligned
    
    ; Reset the floating-point unit
    call reset_fpu
    
    ; Mask all external interrupts
    mov al, 0xff
    out 0xa1, al
    call iodelay
    
    mov al, 0xfb
    out 0x21, al
    call iodelay

    ; Disable interrupts
    cli
    
    ; Enter protected mode
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    
    ; Clear cache
    jmp short next
next:

    ; Jump far to set CS
    jmp dword (SETUP_SEG * 8):code_32
    
code_32:
    ; Setup the stack pointer
    movzx esp, sp
    mov ax, ss
    movzx eax, ax
    shl eax, 4
    add esp, eax
    
    ; Setup the segment registers
    mov ax, (DATA_SEG * 8)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Restore real mode code start address and pass to kernel in esi
    pop esi
    
    ; Jump to the kernel entry point    
    jmp dword (CODE_SEG * 8):BOOT_SETUP32_ADDR
    
    ; This adds only a few bytes (or none). The main reason for this line is to
    ; ensure an error is generated (TIMES value is negative) if the code above
    ; crosses into the e820 memory map.
    times BOOT_SETUP_ADDR(BOOT_E820_MAP) -($-$$) db 0

;------------------------------------------------------------------------
; E820 Memory map
;
; The e820 memory map starts here. The code below gets overwritten by the
; memory map. Only code that runs *before* the memory map is generated
; should go here.
;------------------------------------------------------------------------
e820_map:

;------------------------------------------------------------------------
; FUNCTION:    check_a20
; DESCRIPTION:    Check if A20 gate is activated. Return with ZF=0 if it 
;                is.
;------------------------------------------------------------------------
check_a20:    
    ; Save segment registers
    push ds
    push es
    
    ; Setup the segment registers
    ; If A20 is disabled, ds:0x00 (0000+00) is the same as es:0x10 (FFFF0+10)
    xor ax, ax
    mov ds, ax  ; 0x0000
    dec ax
    mov es, ax  ; 0xFFFF
    
    ; Save everything. This is especially important for es:0x10 since it's the
    ; kernel code we are playing with...
    push dword [ds:0]
    mov eax, [es:0x10]
    push eax
    
    ; Since we will be playing around with the interrupt vector table, lets not 
    ; take chances
    cli
    
    mov cx, 0xFF    
.loop:
    inc eax
    mov [ds:0], eax
    call iodelay
    cmp eax, [es:0x10]
    loopz .loop
    
    ; Restore memory content
    pop dword [es:0x10]
    pop dword [ds:0]
    
    ; Restore segment registers
    pop es
    pop ds
    
    sti
    ret

;------------------------------------------------------------------------
; FUNCTION:    enable_a20
; DESCRIPTION:    Activate the A20 gate.
;------------------------------------------------------------------------
enable_a20:
    ; If we are lucky, there is nothing to do...
    call check_a20
    jnz .done
    
    ; Lets politely ask the BIOS to do this for us
    mov ax, 0x2401
    int 0x15
    call check_a20
    jnz .done
    
    ; Next, try using the keyboard controller
    call .empty_8042
    mov al, 0xd1
    out 0x64, al
    
    call .empty_8042
    mov al, 0xdf
    out 0x60, al
    
    call .empty_8042
    call check_a20
    jnz .done
    
    ; Last resort: fast A20 gate
    in al, 0x92
    test al, 2
    jnz .no_use
    or al, 2
    out 0x92, al
    
    call check_a20
    jnz .done

.no_use:
    jmp short .no_use    

.done:    
    ret

.empty_8042:
    call iodelay
    
    in al, 0x64
    test al, 2
    jnz .empty_8042
    ret
    
    ; Skip to end of e820 memory map
    times BOOT_SETUP_ADDR(BOOT_E820_MAP_END)-($-$$) db 0

;------------------------------------------------------------------------
; End of E820 Memory map
;
; Code below is safe: it does not get overwritten by the e820 memory map.
;------------------------------------------------------------------------
e820_map_end:

;------------------------------------------------------------------------
; FUNCTION:    iodelay
; DESCRIPTION:    Wait for about 1 µs.
;------------------------------------------------------------------------
iodelay:
    out 0x80, al
    ret

;------------------------------------------------------------------------
; FUNCTION:    bios_e820
; DESCRIPTION:    Obtain physical memory map from BIOS.
;------------------------------------------------------------------------
bios_e820:
    mov di, e820_map        ; Buffer (start of memory map)
    mov ebx, 0              ; Continuation set to 0
    cld
    
.loop:
    mov eax, 0xe820         ; Function number e820
    mov ecx, 20             ; Size of buffer, in bytes
    mov edx, E820_SMAP      ; Signature
    
    int 0x15
    
    jc .exit                ; Carry flag set indicates error or end

    cmp eax, E820_SMAP      ; EAX should contain signature
    jne .exit

    cmp ecx, 20             ; 20 bytes should have been returned
    jne .exit
    
    or ebx, ebx             ; EBX (maybe) set to 0 on end of map
    jz .exit
    
    add di, 20              ; Go to next entry in memory map
    
    mov ax, di              ; Check that we can still fit one more entry
    add ax, 20
    cmp ax, e820_map_end
    jbe .loop
    
.exit:
    ; Compute number of entries
    mov ax, di              ; Size is end ...
    sub ax, e820_map        ; ... minus start
    xor dx, dx              ; High 16-bits of dx:ax (i.e. dx) are zero
    mov bx, 20              ; Divide by 20
    div bx
    
    ; The e820_entries field is not in our current segment. Change segment to
    ; the the previous 512-byte sector (see boot.asm).
    push es
    
    mov bx, es
    sub bx, 0x20
    mov es, bx
    
    ; Set number of entries
    mov byte [es:BOOT_E820_ENTRIES], al
    
    ; Restore ES
    pop es
        
    ret

;------------------------------------------------------------------------
; FUNCTION:    reset_fpu
; DESCRIPTION:    Reset the floating-point unit.
;------------------------------------------------------------------------
reset_fpu:
    xor ax, ax
    out 0xf0, al
    call iodelay
    
    out 0xf1, al
    call iodelay
    
    ret

;------------------------------------------------------------------------
; Data section
;------------------------------------------------------------------------
str_version:
    db "Test kernel v0.0", 0


;------------------------------------------------------------------------
; Global Descriptor Table (minimal)
;------------------------------------------------------------------------
    align 16

gdt:
.null:  dd 0, 0
.code:  dw 0xffff, 0, 0x9a00, 0x00cf 
.data:  dw 0xffff, 0, 0x9200, 0x00cf 
.setup: dw 0xffff, 0, 0x9a00, 0x0000
.end:

    align 4
    dw 0
    
gdt_info:
.limit: dw gdt.end - gdt - 1
.addr:  dd 0            ; Patched in later

    align 512
