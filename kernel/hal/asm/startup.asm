    bits 32

    extern kmain
    extern boot_info
    
    global _start:function (_start.end - _start)
_start:
    ; The 32-bit setup code sets esi to the boot information structure.
    ;
    ; See boot/setup32.asm.
    mov dword [boot_info], esi
    
    ; Call kernel
    call kmain

.end:

    ; The halt() function never returns.
    ;
    ; If kmain() ever returns, it will enter halt().
    global halt:function    (halt.end - halt)
halt:
    cli
    hlt
    jmp halt

.end:
