    bits 32
    
; ------------------------------------------------------------------------------
; FUNCTION: get_fpointer
; C PROTOTYPE: addr_t get_fpointer(void)
; ------------------------------------------------------------------------------
    global get_fpointer
get_fpointer:
    mov eax, ebp
    
    ret


; ------------------------------------------------------------------------------
; FUNCTION: get_caller_fpointer
; C PROTOTYPE: get_caller_fpointer(addr_t fptr)
; ------------------------------------------------------------------------------
    global get_caller_fpointer
get_caller_fpointer:
    push ebp                ; Save ebp
    
    mov ebp, [esp+8]        ; First argument: fptr
    mov eax, [ebp]          ; Frame pointer to return
    
    pop ebp                 ; Restore ebp
    ret


; ------------------------------------------------------------------------------
; FUNCTION: get_ret_addr
; C PROTOTYPE: addr_t get_ret_addr(addr_t fptr)
; ------------------------------------------------------------------------------
    global get_ret_addr
get_ret_addr:
    push ebp                ; Save ebp
    
    mov ebp, [esp+8]        ; First argument: fptr
    mov eax, [ebp+4]        ; Return address to return
    pop ebp                 ; Restore ebp
    
    ret


; ------------------------------------------------------------------------------
; FUNCTION: get_program_counter
; C PROTOTYPE: addr_t get_program_counter(void)
; ------------------------------------------------------------------------------
    global get_program_counter
get_program_counter:
    mov eax, [esp]
    
    ret
