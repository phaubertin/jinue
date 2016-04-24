%define GDT_KERNEL_DATA         2
%define GDT_LENGTH              8

    bits 32
    
    global syscall_method
    extern dispatch_syscall
    extern in_kernel

; ------------------------------------------------------------------------------
; FUNCTION: fast_intel_entry
; ------------------------------------------------------------------------------
    global fast_intel_entry
fast_intel_entry:
    ; save return address and user stack
    push ebp    ; user stack
    push ecx    ; return address
    
    ; dummy frame pointer
    mov ebp, 0
    
    ; save data segment selectors
    push ds
    push es
    push gs
    
    ; set data segment
    mov ecx, GDT_KERNEL_DATA * 8 + 0
    mov ds, cx
    mov es, cx
    
    ; set per-cpu data segment
    ;
    ; The entry which follows the TSS descriptor in the GDT is a data
    ; segment which points to per-cpu data, including the TSS.
    str cx          ; get selector for TSS descriptor
    add cx, 8       ; next entry
    mov gs, cx      ; load gs with data segment selector
    
    ; syscall_params_t structure
    push edi    ; 16 args.arg2
    push esi    ; 12 args.arg1
    push eax    ;  8 args.funct
    push edx    ;  4 args.method
    push ebx    ;  0 args.dest
    
    ; push a pointer to the previous structure as parameter to
    ; the hal_syscall_dispatch function
    push esp
    
    ; entering the kernel
    mov [in_kernel], dword 1
    
    call dispatch_syscall
    
    ; leaving the kernel
    mov [in_kernel], dword 0
    
    ; setup return value
    mov ebx, [esp+4+0]      ;  0 ret.errno
    mov eax, [esp+4+8]      ;  8 ret.val
    mov edi, [esp+4+16]     ; 16 ret.errno
    
    ; cleanup stack
    add esp, 24
    
    ; restore data segment selectors
    pop gs
    pop es
    pop ds
    
    ; restore return address and user stack
    pop edx        ; return address
    pop ecx        ; user stack
    
    sysexit

; ------------------------------------------------------------------------------
; FUNCTION: fast_amd_entry
; ------------------------------------------------------------------------------
    global fast_amd_entry
fast_amd_entry:
    ; save user stack pointer temporarily in ebp
    mov ebp, esp
    
    ; set per-cpu data segment and get kernel stack pointer from TSS
    ;
    ; The entry which follows the TSS descriptor in the GDT is a data
    ; segment which points to per-cpu data, including the TSS.
    str sp                              ; get selector for TSS descriptor
    add sp, 8                           ; next entry
    mov gs, sp                          ; load gs with data segment selector
    mov esp, [gs:GDT_LENGTH * 8 + 4]    ; load kernel stack pointer from TSS
                                        ; Stack pointer is at offset 4 in the TSS, and
                                        ; the TSS follows the GDT (see cpu_data_t).
    
    ; save return address and user stack
    push ebp    ; user stack
    push ecx    ; return address
    
    ; dummy frame pointer
    mov ebp, 0
    
    ; save data segment selectors
    push ds
    push es
    
    ; set data segment
    mov ecx, GDT_KERNEL_DATA * 8 + 0
    mov ds, cx
    mov es, cx
    
    ; syscall_params_t structure
    push edi    ; 16 args.arg2
    push esi    ; 12 args.arg1
    push eax    ;  8 args.funct
    push edx    ;  4 args.method
    push ebx    ;  0 args.dest
    
    ; push a pointer to the previous structure as parameter to
    ; the hal_syscall_dispatch function
    push esp
    
    ; entering the kernel
    mov [in_kernel], dword 1
    
    call dispatch_syscall
    
    ; leaving the kernel
    mov [in_kernel], dword 0

    ; setup return value
    mov ebx, [esp+4+0]      ;  0 ret.errno
    mov eax, [esp+4+8]      ;  8 ret.val
    mov edi, [esp+4+16]     ; 16 ret.errno
    
    ; cleanup stack
    add esp, 24
    
    ; restore data segment selectors
    pop es
    pop ds
    
    ; we don't want user space to have our per-cpu data segment selector
    mov ecx, 0
    mov gs, cx
    
    ; restore return address and user stack
    pop ecx         ; return address
    pop esp         ; user stack
    
    sysret

; ------------------------------------------------------------------------------
; System call method
; ------------------------------------------------------------------------------
    section .rodata

syscall_method:
    dd 0
