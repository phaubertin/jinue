    bits 32
    
    extern main
    extern exit
    extern auxvp

; ------------------------------------------------------------------------------
; ELF binary entry point
; ------------------------------------------------------------------------------    
    global crt_start
crt_start:
    ; we are going up
    cld
    
    ; keep a copy of address of argc before we modify the stack pointer
    mov esi, esp
    
    ; set frame pointer and move stack pointer to the end of the stack frame
    mov ebp, esp
    sub esp, 12
    
    ; Store argc as first argument to main while keeping a copy in eax. 
    ; Register esi is incremented by this operation to point on argv.
    lodsd
    mov dword [ebp-12], eax
    
    ; Store argv as second argument to main
    mov dword [ebp-8], esi
    
    ; Skip whole argv array (eax entries) as well as the following NULL pointer
    lea esi, [esi + 4 * eax + 4]
    
    ; Store envp as third argument to main
    mov dword [ebp-4], esi
    
    ; Increment esi (by 4 each time) until we find the NULL pointer which marks
    ; the end of the environment variables. Since esi is post-incremented, it
    ; will point at the address past the NULL pointer at the end of the loop,
    ; which is the start of the auxiliary vectors.
    xor eax, eax
    mov edi, esi
    repnz scasd
    
    ; Set address of auxiliary vectors
    mov dword [auxvp], edi
    
    ; Now that all arguments are where they should be, call main()
    call main
    
    ; main() must never return
