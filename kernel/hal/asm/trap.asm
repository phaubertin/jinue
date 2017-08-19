#include <hal/asm/descriptors.h>
#include <hal/asm/irq.h>

%define NO_ERROR_CODE                   0
%define GOT_ERROR_CODE                  1

    bits 32

    extern dispatch_interrupt
    extern dispatch_syscall
    extern in_kernel

; ------------------------------------------------------------------------------
; FUNCTION: interrupt_entry
; DESCRIPTION : Save current thread state, call interrupt dispatching function,
;               then restore state and return from interrupt.
; ------------------------------------------------------------------------------
    global interrupt_entry:function (return_from_interrupt.end - interrupt_entry)
interrupt_entry:
    ; We use the version of the push instruction with a byte operand in the
    ; jump table because it is the shortest form of this instruction (2 bytes).
    ; However, the byte operand is sign extended by the instruction, which is
    ; obviously not what we want. Here, we mask the most significant bits of
    ; the interrupt vector to make it zero-extended instead.
    and dword [esp], 0xff
    
    cld
    
    ; save thread state on stack
    push gs   ; 36
    push fs   ; 32
    push es   ; 28
    push ds   ; 24
    push ecx  ; 20
    push edx  ; 16

    ; system call arguments (pushed in reverse order)
    ;
    ; The interrupt dispatcher is passed a pointer to these four arguments.
    ; If this interrupt request turns out to be a system call, this pointer
    ; gets passed to the system call dispatcher where the values pushed here
    ; may be modified.
    push edi  ; 12  arg3
    push esi  ; 8   arg2
    push ebx  ; 4   arg1
    push eax  ; 0   arg0
    
    ; set data segment
    mov eax, GDT_KERNEL_DATA * 8
    mov ds, ax
    mov es, ax
    
    ; set per-cpu data segment (TSS)
    ;
    ; The entry which follows the TSS descriptor in the GDT is a data segment
    ; which also points to the TSS (same base address and limit).
    str ax              ; get selector for TSS descriptor
    add ax, 8           ; next entry
    mov gs, ax          ; load gs with data segment selector
    
    ; We want to save the frame pointer for debugging purposes, to
    ; ensure we have a complete call stack dump if we need it.
    ;
    ; If we were executing in-kernel before the interrupt/exception
    ; occured, we save the frame pointer (ebp), which means we can
    ; actually know what the kernel was doing at the time the interrupt
    ; happened. If, on the other hand, the cpu was executing user code
    ; at the time, ebp is neither trustworthy nor useful, so we store
    ; nothing and set ebp to zero to terminate the chain instead.
    ;
    ; We need to store the frame pointer right after the return address.
    ; So, we will be overwriting the interrupt vector, unless the
    ; interrupt was actually an exception with an error code, in which
    ; case, it is the error code that we will be overwriting.
    
    ; keep old value of ebp around to store it later
    mov ebx, ebp
    
    ; default values
    mov edx, 0              ; error code (default)
    mov ecx, NO_ERROR_CODE  ; there is no error code (default)
    mov ebp, 0              ; new frame pointer (default)
    
    ; get interrupt vector    
    mov edi, esp
    add edi, 40
    mov eax, [edi]
    
    ; exceptions 8 (double fault) and 17 (alignment) both have an
    ; error code
    cmp eax, EXCEPTION_DOUBLE_FAULT
    jz .has_error_code
    
    cmp eax, EXCEPTION_ALIGNMENT
    jz .has_error_code
    
    ; exceptions 10 (invalid TSS) to 14 (page fault) all have an
    ; error code
    cmp eax, EXCEPTION_INVALID_TSS 
    jl .no_error_code
    
    cmp eax, EXCEPTION_PAGE_FAULT 
    jg .no_error_code

.has_error_code:
    ; update frame pointer address to overwrite error code instead of
    ; the ivt
    add edi, 4
    
    ; get error code
    mov edx, [edi]
    
    ; remember that we have an error code
    mov ecx, GOT_ERROR_CODE

.no_error_code:

    ; check if we were in kernel before the interrupt occurred
    mov esi, [in_kernel]
    or esi, esi
    jz .not_in_kernel   ; not in kernel if non-zero

.indeed_in_kernel:
    
    ; set frame pointer for this stack frame
    mov ebp, edi

.not_in_kernel:

    ; store frame pointer...
    ; ... which is the original value of ebp...
    ; ... which we hadn't stored yet
    mov [edi], ebx
    
    ; remember whether we have an error code or not
    push ecx
    
    ; (re)-entering the kernel
    push dword [in_kernel]
    inc dword [in_kernel]
    
    ; set function parameters
    mov  ecx, esp
    add  ecx, 8
    push ecx            ; Fourth param: system call parameters (for slow system call method)
    push edx            ; Third param:  error code
    push dword [edi+4]  ; Second param: return address (eip)
    push eax            ; First param:  Interrupt vector
    
    ; call interrupt dispatching function
.dispatch:
    call dispatch_interrupt
    
    ; remove parameters from stack
    add esp, 16

    ; new threads start here
    global return_from_interrupt
return_from_interrupt:

    ; restore in_kernel
    pop dword [in_kernel]

    ; error code/no error code
    pop ebp
    
    ; restore thread state
    ;
    ; These first four registers are the system call arguments (and return
    ; values). If this interrupt request is a system call, the values we are
    ; popping here are (likely) not the same as the ones we pushed at the
    ; beginning of this function.
    pop eax     ; arg0
    pop ebx     ; arg1
    pop esi     ; arg2
    pop edi     ; arg3

    pop edx
    pop ecx
    pop ds
    pop es
    pop fs
    pop gs
    
    ; try to remember where we left ebp
    cmp ebp, GOT_ERROR_CODE
    jne .no_error_code2
    
    add esp, 4
    
.no_error_code2:
    ; restore ebp
    pop ebp
    
    ; return from interrupt
    iret

.end:

; ------------------------------------------------------------------------------
; FUNCTION: fast_intel_entry
; ------------------------------------------------------------------------------
    global fast_intel_entry:function (fast_intel_entry.end - fast_intel_entry)
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

.end:

; ------------------------------------------------------------------------------
; FUNCTION: fast_amd_entry
; ------------------------------------------------------------------------------
    global fast_amd_entry:function (fast_amd_entry.end - fast_amd_entry)
fast_amd_entry:
    ; save user stack pointer temporarily in ebp
    mov ebp, esp
    
    ; set per-cpu data segment (in gs) and get kernel stack pointer from TSS
    ;
    ; The entry which follows the TSS descriptor in the GDT is a data
    ; segment which points to per-cpu data, including the TSS.
    ;
    ; Kernel calling convention: the calling code is responsible for saving the
    ; gs segment selector before calling into the kernel with the SYSCALL
    ; instruction.
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

.end:

; ------------------------------------------------------------------------------
; Interrupt Jump Table (irq_jtable)
; ------------------------------------------------------------------------------
section .text
align 32

    global irq_jtable
irq_jtable:
    
    %assign ivt 0
    %rep IDT_VECTOR_COUNT / 7 + 1
        %assign stone_ivt ivt
        %define stone .jump %+ stone_ivt
stone:
        jmp interrupt_entry
        
        %rep 7
            %if ivt < IDT_VECTOR_COUNT
                ; set irq_jtable.irqxx label
                .irq %+ ivt:
                
                ; This if statement is not technically necessary, but it
                ; prevents the assembler from warning that the operand is
                ; out of bound for vectors > 127.
                %if ivt < 128
                    push byte ivt
                %else
                    push byte ivt-256
                %endif
                
                jmp short stone
                
                %assign ivt ivt+1
            %endif
        %endrep
        
    %endrep

; ------------------------------------------------------------------------------
; Interrupt Vector Table (IDT)
; ------------------------------------------------------------------------------
; Here, we reserve enough space for the IDT in the .data section (64 bits per
; entry/descriptor) and we store in each entry the address of the matching
; jump table stub. The kernel initialization code will re-process this table
; later and create actual interrupt gate descriptors.

section .data
align 32

    global idt
idt:
    %assign ivt 0
    %rep IDT_VECTOR_COUNT
        ; set to irq_jtable.irqxx label
        dd irq_jtable.irq %+ ivt
        dd 0
        %assign ivt ivt+1
    %endrep
