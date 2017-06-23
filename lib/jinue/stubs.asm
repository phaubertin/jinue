    bits 32
    
    section .text    
; ------------------------------------------------------------------------------
; FUNCTION: syscall_fast_intel
; C PROTOTYPE: void syscall_fast_intel(jinue_syscall_args_t *args);
; ------------------------------------------------------------------------------
    global syscall_fast_intel
syscall_fast_intel:
    ; System V ABI calling convention: these four registers must be preserved
    push ebx
    push esi
    push edi
    push ebp
    
    ; At this point, the stack looks like this:
    ;
    ; esp+20  args pointer (first function argument)
    ; esp+16  return address
    ; esp+12  ebx
    ; esp+ 8  esi
    ; esp+ 4  edi
    ; esp+ 0  ebp

    ; first function argument: pointer to system call arguments structure
    mov edi, [esp+20]

	; kernel calling convention: load system call arguments in registers eax,
	; ebx, esi and edi
    mov eax, [edi+ 0]   ; arg0 (function/system call number)
    mov ebx, [edi+ 4]   ; arg1 (target descriptor)
    mov esi, [edi+ 8]   ; arg2 (message pointer)
    mov edi, [edi+12]  	; arg3 (message size)

    ; kernel calling convention: save return address and stack pointer in ecx
    ; and ebp
    ;
    ; This only applies when entering the kernel through the SYSENTER instruction.
    mov ecx, .return_here
    mov ebp, esp
        
    sysenter

.return_here:
    ; restore arguments structure pointer
    mov ebp, [esp+20]
    
    ; store the system call return values back into the arguments structure
    mov [ebp+ 0], eax   ; arg0 (system call-specific/return value)
    mov [ebp+ 4], ebx   ; arg1 (system call-specific/error number)
    mov [ebp+ 8], esi   ; arg2 (system call-specific/reserved)
    mov [ebp+12], edi  	; arg3 (system call-specific/reserved)
    
    pop ebp
    pop edi
    pop esi
    pop ebx

    ret

; ------------------------------------------------------------------------------
; FUNCTION: syscall_fast_amd
; C PROTOTYPE: void syscall_fast_amd(jinue_syscall_args_t *args);
; ------------------------------------------------------------------------------
    global syscall_fast_amd
syscall_fast_amd:
    ; System V ABI calling convention: these four registers must be preserved
    push ebx
    push esi
    push edi
    push ebp

    ; kernel calling convention: the kernel does not preserve gs
    ;
    ; This only applies when entering the kernel through the SYSCALL instruction.
    push gs
    
	; At this point, the stack looks like this:
    ;
    ; esp+24  args pointer (first function argument)
    ; esp+20  return address
    ; esp+16  ebx
    ; esp+12  esi
    ; esp+ 8  edi
    ; esp+ 4  ebp
    ; esp+ 0  gs

    ; first function argument: pointer to system call arguments structure
    mov edi, [esp+24]

    ; Kernel calling convention: load system call arguments in registers eax,
    ; ebx, esi and edi
    mov eax, [edi+ 0]   ; arg0 (function/system call number)
    mov ebx, [edi+ 4]   ; arg1 (target descriptor)
    mov esi, [edi+ 8]   ; arg2 (message pointer)
    mov edi, [edi+12]  	; arg3 (message size)
    
    syscall
    
    ; restore arguments structure pointer
    mov ebp, [esp+24]
    
    ; store the system call return values back into the arguments structure
    mov [ebp+ 0], eax   ; arg0 (system call-specific/return value)
    mov [ebp+ 4], ebx   ; arg1 (system call-specific/error number)
    mov [ebp+ 8], esi   ; arg2 (system call-specific/reserved)
    mov [ebp+12], edi  	; arg3 (system call-specific/reserved)

    pop gs
    pop ebp
    pop edi
    pop esi
    pop ebx

    ret

; ------------------------------------------------------------------------------
; FUNCTION: syscall_intr
; C PROTOTYPE: void syscall_intr(jinue_syscall_args_t *args);
; ------------------------------------------------------------------------------
    global syscall_intr
syscall_intr:
    ; System V ABI calling convention: these four registers must be preserved.
    push ebx
    push esi
    push edi
    push ebp
    
    ; At this point, the stack looks like this:
    ;
    ; esp+20  args pointer (first function argument)
    ; esp+16  return address
    ; esp+12  ebx
    ; esp+ 8  esi
    ; esp+ 4  edi
    ; esp+ 0  ebp

    ; first function argument: pointer to system call arguments structure
    mov edi, [esp+20]

    ; kernel calling convention: load system call arguments in registers eax,
    ; ebx, esi and edi
    mov eax, [edi+ 0]   ; arg0 (function/system call number)
    mov ebx, [edi+ 4]   ; arg1 (target descriptor)
    mov esi, [edi+ 8]   ; arg2 (message pointer)
    mov edi, [edi+12]  	; arg3 (message size)

    int 0x80
    
    ; restore arguments structure pointer
    mov ebp, [esp+20]
    
    ; store the system call return values back into the arguments structure
    mov [ebp+ 0], eax   ; arg0 (system call-specific/return value)
    mov [ebp+ 4], ebx   ; arg1 (system call-specific/error number)
    mov [ebp+ 8], esi   ; arg2 (system call-specific/reserved)
    mov [ebp+12], edi  	; arg3 (system call-specific/reserved)

    pop ebp
    pop edi
    pop esi
    pop ebx

    ret
