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
    ; save return address and user stack pointer
    ;
    ; kernel calling convention: the calling code must store these in the ebp
    ; and ecx registers before executing the SYSENTER instruction.
    push ebp        ; user stack pointer
    push ecx        ; return address
    
    ; dummy frame pointer
    mov ebp, 0
    
    ; save data segment selectors
    push ds
    push es
    push gs
    
    ; set data segment
    mov ecx, GDT_KERNEL_DATA * 8
    mov ds, cx
    mov es, cx
    
    ; set per-cpu data segment
    ;
    ; The entry which follows the TSS descriptor in the GDT is a data
    ; segment which points to per-cpu data, including the TSS.
    str cx          ; get selector for TSS descriptor
    add cx, 8       ; next entry
    mov gs, cx      ; load gs with data segment selector
    
    ; system call arguments (pushed in reverse order)
    push edi        ; arg3
    push esi        ; arg2
    push ebx        ; arg1
    push eax        ; arg0
    
    ; push a pointer to the structure above as an argument to dispatch_syscall
    push esp
    
    ; At this point, the stack looks like this:
    ;
    ; esp+36  user stack pointer
    ; esp+32  user return address
    ; esp+28  ds
    ; esp+24  es
    ; esp+20  gs
    ; esp+16  edi (system call arg3)
    ; esp+12  esi (system call arg2)
    ; esp+ 8  ebx (system call arg1)
    ; esp+ 4  eax (system call arg0)
    ; esp+ 0  pointer to message arguments (first dispatch_syscall argument)

    ; entering the kernel
    mov [in_kernel], dword 1
    
    call dispatch_syscall
    
    ; leaving the kernel
    mov [in_kernel], dword 0
    
    ; cleanup dispatch_syscall argument
    add esp, 4
    
    ; system call return values
    pop eax         ; arg0
    pop ebx         ; arg1
    pop esi         ; arg2
    pop edi         ; arg3
    
    ; restore data segment selectors
    pop gs
    pop es
    pop ds
    
    ; restore return address and user stack
    ;
    ; The SYSEXIT instruction requires these to be in the edx and ecx registers.
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
    
    ; set per-cpu data segment (in gs) and get kernel stack pointer from TSS
    ;
    ; The entry which follows the TSS descriptor in the GDT is a data
    ; segment which points to per-cpu data, including the TSS.
    ;
    ; Kernel calling convention: the calling code is responsible for saving the
    ; gs segment selector before calling into the kernel.
    str sp                              ; get selector for TSS descriptor
    add sp, 8                           ; next entry
    mov gs, sp                          ; load gs with data segment selector
    mov esp, [gs:GDT_LENGTH * 8 + 4]    ; load kernel stack pointer from TSS
                                        ; Stack pointer is at offset 4 in the TSS, and
                                        ; the TSS follows the GDT (see cpu_data_t).
    
    ; save return address and user stack
    ;
    ; The return address is moved to ecx by the SYSCALL instruction.
    push ebp    ; user stack pointer
    push ecx    ; return address
    
    ; dummy frame pointer
    mov ebp, 0
    
    ; save data segment selectors
    push ds
    push es
    
    ; set data segment
    mov ecx, GDT_KERNEL_DATA * 8
    mov ds, cx
    mov es, cx
    
    ; system call arguments (pushed in reverse order)
    push edi        ; arg3
    push esi        ; arg2
    push ebx        ; arg1
    push eax        ; arg0
    
    ; push a pointer to the structure above as an argument to dispatch_syscall
    push esp
    
    ; At this point, the stack looks like this:
    ;
    ; esp+32  user stack pointer
    ; esp+28  user return address
    ; esp+24  ds
    ; esp+20  es
    ; esp+16  edi (system call arg3)
    ; esp+12  esi (system call arg2)
    ; esp+ 8  ebx (system call arg1)
    ; esp+ 4  eax (system call arg0)
    ; esp+ 0  pointer to message arguments (dispatch_syscall first argument)

    ; entering the kernel
    mov [in_kernel], dword 1
    
    call dispatch_syscall
    
    ; leaving the kernel
    mov [in_kernel], dword 0

    ; cleanup dispatch_syscall argument
    add esp, 4
    
    ; system call return values
    pop eax         ; arg0
    pop ebx         ; arg1
    pop esi         ; arg2
    pop edi         ; arg3
    
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
