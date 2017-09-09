; Copyright (C) 2017 Philippe Aubertin.
; All rights reserved.
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions
; are met:
; 
; 1. Redistributions of source code must retain the above copyright
;    notice, this list of conditions and the following disclaimer.
; 
; 2. Redistributions in binary form must reproduce the above copyright
;    notice, this list of conditions and the following disclaimer in the
;    documentation and/or other materials provided with the distribution.
; 
; 3. Neither the name of the author nor the names of other contributors
;    may be used to endorse or promote products derived from this software
;    without specific prior written permission.
; 
; THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND
; ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
; WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
; DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY
; DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
; (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
; ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
; (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
; SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <hal/asm/descriptors.h>
#include <hal/asm/irq.h>


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
    cld
    
    ; Once everything is saved and after some reshuffling, the stack layout
    ; matches the trapframe_t structure definition. It looks like this:
    ;
    ; esp+72  user stack segment
    ; esp+68  user stack pointer
    ; esp+64  user EFLAGS
    ; esp+60  user code segment
    ; esp+56  user return address
    ;
    ; esp+52  ebp (but the error code is here on entry, see below)
    ; esp+48  interrupt vector
    ;
    ; esp+44  error code
    ; esp+40  gs
    ; esp+36  fs
    ; esp+32  es
    ; esp+28  ds
    ; esp+24  ecx
    ; esp+20  edx
    ; esp+16  [in_kernel]
    ; esp+12  edi (message/system call argument 3)
    ; esp+ 8  esi (message/system call argument 2)
    ; esp+ 4  ebx (message/system call argument 1)
    ; esp+ 0  eax (message/system call argument 0)
    
    sub esp, 4  ; 44 reserve space for the error code

    push gs     ; 40
    push fs     ; 36
    push es     ; 32
    push ds     ; 28
    push ecx    ; 24
    push edx    ; 20
    
    ; set data segment
    mov ecx, SEG_SELECTOR(GDT_KERNEL_DATA, RPL_KERNEL)
    mov ds, cx
    mov es, cx
    
    ; Indicates whether this interrupt occurred in userspace (== 0) or in the
    ; kernel (> 0).
    ;
    ; We keep a copy in ecx for later use
    mov ecx, dword [in_kernel]
    push ecx    ; 16
    
    ; system call arguments (pushed in reverse order)
    ;
    ; The kernel modifies these to set return values.
    push edi    ; 12 arg3
    push esi    ; 8  arg2
    push ebx    ; 4  arg1
    push eax    ; 0  arg0
    
    ; We use the version of the push instruction with a byte operand in the
    ; interrupt vector stubs because it is the shortest form of this instruction
    ; (2 bytes). However, the byte operand is sign extended by the instruction,
    ; which is obviously not what we want. Here, we mask the most significant
    ; bits of the interrupt vector to make it zero-extended instead.
    and dword [esp+48], 0xff
    
    ; If we are handling a CPU exception with an error code, the CPU pushed
    ; that error code right after the return address. Otherwise, the interrupt
    ; vector stub pushed a dummy value there to keep the same stack layout.
    ;
    ; However, we want to put ebp there because, if we are handling an interrupt
    ; that happened in the kernel, ebp contains the frame pointer. This makes
    ; debugging easier because it allows us to include what the kernel was 
    ; doing when the interrupt was triggered in the backtrace.
    mov edx, [esp+52]   ; read error code
    mov [esp+44], edx   ; save it where we actually want it
    mov [esp+52], ebp   ; save ebp (frame pointer) right after the return address
    
    ; Clear frame pointer until we know for sure we were in the kernel when
    ; the interrupt occurred. If the CPU was executing user code at the time,
    ; ebp is neither trustworthy nor useful.
    mov ebp, 0
    
    ; Check to see if we were in the kernel before the interrupt.
    ;
    ; ecx contains the value of [in_kernel].
    or ecx, ecx
    jz .skip_fp
    
    ; set frame pointer
    lea ebp, [esp+52]
    
.skip_fp:
    ; (re)-entering the kernel
    inc ecx
    mov dword [in_kernel], ecx
    
    ; set per-cpu data segment
    mov eax, SEG_SELECTOR(GDT_PER_CPU_DATA, RPL_KERNEL)
    mov gs, ax
    
    ; set dispatch_interrupt() function argument
    push esp            ; First argument:  trapframe
    
    ; call interrupt dispatching function
    call dispatch_interrupt
    
    ; remove argument(s) from stack
    add esp, 4

    ; new threads start here
    global return_from_interrupt
return_from_interrupt:
    
    pop eax                 ; 0
    pop ebx                 ; 4
    pop esi                 ; 8
    pop edi                 ; 12
    pop dword [in_kernel]   ; 16
    pop edx                 ; 20
    pop ecx                 ; 24
    pop ds                  ; 28
    pop es                  ; 32
    pop fs                  ; 36
    pop gs                  ; 40
    add esp, 8              ; 44 skip error code
                            ; 48 skip interrupt vector
    pop ebp                 ; 52
    
    ; return from interrupt
    iret

.end:

; ------------------------------------------------------------------------------
; FUNCTION: fast_intel_entry
; ------------------------------------------------------------------------------
    global fast_intel_entry:function (fast_intel_entry.end - fast_intel_entry)
fast_intel_entry:
    ; kernel calling convention: before executing the SYSENTER instruction, the
    ; calling code must store:
    ;   - The user return address in ecx
    ;   - The user stack pointer in ebp
    ;
    ; For details on the stack layout, see comments in interrupt_entry above and
    ; the definition of the trapframe_t type.
    
    push byte SEG_SELECTOR(GDT_USER_DATA, RPL_USER)     ; 72
    push ebp                                            ; 68 user stack pointer
    pushf                                               ; 64
    push byte SEG_SELECTOR(GDT_USER_CODE, RPL_USER)     ; 60
    push ecx                                            ; 56 user return address
    
    mov ebp, 0              ; setup dummy frame pointer
    
    push byte 0             ; 52 ebp (caller-saved by kernel calling convention)
    push byte 0             ; 48 interrupt vector (unused)
    push byte 0             ; 44 error code (unused)
    push gs                 ; 40
    push fs                 ; 36
    push es                 ; 32
    push ds                 ; 28
    push byte 0             ; 24 ecx (caller-saved by System V ABI)
    push byte 0             ; 20 edx (caller-saved by System V ABI)
    push byte 0             ; 16 [in_kernel]
    
    ; system call arguments (pushed in reverse order)
    ;
    ; The kernel modifies these to set return values.
    push edi                ; 12 arg3
    push esi                ; 8  arg2
    push ebx                ; 4  arg1
    push eax                ; 0  arg0
    
    ; set data segment
    mov ecx, SEG_SELECTOR(GDT_KERNEL_DATA, RPL_KERNEL)
    mov ds, cx
    mov es, cx
    
    ; set per-cpu data segment
    mov eax, SEG_SELECTOR(GDT_PER_CPU_DATA, RPL_KERNEL)
    mov gs, ax

    ; entering the kernel
    mov [in_kernel], dword 1
    
    ; set dispatch_syscall() function argument
    push esp                ; First argument:  trapframe
    
    call dispatch_syscall
    
    ; cleanup dispatch_syscall() argument
    add esp, 4
    
    pop eax                 ; 0
    pop ebx                 ; 4
    pop esi                 ; 8
    pop edi                 ; 12
    pop dword [in_kernel]   ; 16
    add esp, 8              ; 20 skip edx (used for stack pointer by SYSEXIT)
                            ; 24 skip ecx (used for return address by SYSEXIT)
    pop ds                  ; 28
    pop es                  ; 32
    pop fs                  ; 36
    pop gs                  ; 40
    
    add esp, 8              ; 44 skip error code
                            ; 48 skip interrupt vector
    pop ebp                 ; 52
    pop edx                 ; 56 return address
    add esp, 4              ; 60 skip user code segment
    popf                    ; 64
    pop ecx                 ; 68 user stack pointer
    ; no action needed      ; 72 skip user stack segment
    
    sysexit

.end:

; ------------------------------------------------------------------------------
; FUNCTION: fast_amd_entry
; ------------------------------------------------------------------------------
    global fast_amd_entry:function (fast_amd_entry.end - fast_amd_entry)
fast_amd_entry:
    ; save user stack pointer temporarily in ebp
    ;
    ; Kernel calling convention: the calling code is responsible for saving ebp
    ; before calling into the kernel with the SYSCALL instruction.
    mov ebp, esp
    
    ; set per-cpu data segment (in gs) and get kernel stack pointer from TSS
    ;
    ; Kernel calling convention: the calling code is responsible for saving the
    ; gs segment selector before calling into the kernel with the SYSCALL
    ; instruction.
    mov edx, SEG_SELECTOR(GDT_PER_CPU_DATA, RPL_KERNEL)
    mov gs, dx                          ; load gs with per-cpu data segment selector
    mov esp, [gs:GDT_LENGTH * 8 + 4]    ; load kernel stack pointer from TSS
                                        ; Stack pointer is at offset 4 in the TSS, and
                                        ; the TSS follows the GDT (see cpu_data_t).
    
    ; For details on the stack layout, see comments in interrupt_entry above and
    ; the definition of the trapframe_t type.
    
    push byte SEG_SELECTOR(GDT_USER_DATA, RPL_USER)     ; 72
    push ebp                                            ; 68 user stack pointer
    pushf                                               ; 64
    push byte SEG_SELECTOR(GDT_USER_CODE, RPL_USER)     ; 60
    push ecx                                            ; 56 user return address
    
    mov ebp, 0              ; setup dummy frame pointer
    
    push byte 0             ; 52 ebp (caller-saved by kernel calling convention)
    push byte 0             ; 48 interrupt vector (unused)
    push byte 0             ; 44 error code (unused)
    push byte 0             ; 40 gs (caller-saved by kernel calling convention)
    push fs                 ; 36
    push es                 ; 32
    push ds                 ; 28
    push byte 0             ; 24 ecx (caller-saved by System V ABI)
    push byte 0             ; 20 edx (caller-saved by System V ABI)
    push byte 0             ; 16 [in_kernel]
    
    ; system call arguments (pushed in reverse order)
    ;
    ; The kernel modifies these to set return values.
    push edi                ; 12 arg3
    push esi                ; 8  arg2
    push ebx                ; 4  arg1
    push eax                ; 0  arg0
    
    ; set data segment
    mov ecx, SEG_SELECTOR(GDT_KERNEL_DATA, RPL_KERNEL)
    mov ds, cx
    mov es, cx

    ; entering the kernel
    mov [in_kernel], dword 1
    
    ; set dispatch_syscall() function argument
    push esp                ; First argument:  trapframe
    
    call dispatch_syscall
    
    ; cleanup dispatch_syscall() argument
    add esp, 4
    
    pop eax                 ; 0
    pop ebx                 ; 4
    pop esi                 ; 8
    pop edi                 ; 12
    pop dword [in_kernel]   ; 16
    pop edx                 ; 20
    add esp, 4              ; 24 skip ecx (used for return address by SYSRET)
    pop ds                  ; 28
    pop es                  ; 32
    pop fs                  ; 36
    pop gs                  ; 40
    add esp, 8              ; 44 skip error code
                            ; 48 skip interrupt vector
    pop ebp                 ; 52
    pop ecx                 ; 56 return address
    add esp, 4              ; 60 skip user code segment
    popf                    ; 64
    pop esp                 ; 68 user stack pointer
    ; no action needed      ; 72 skip user stack segment
    
    sysret

.end:

; ------------------------------------------------------------------------------
; Interrupt Stubs (irq_jtable)
; ------------------------------------------------------------------------------
; A stub is generated for each interrupt vector that pushes the interrupt number
; on the stack and then jumps to the interrupt entry point (interrupt_entry).

    section .text
    align 32
    
    %define NULL_ERRCODE    0
    %define PER_BLOCK       15

    global irq_jtable
irq_jtable:
    
    %assign ivt 0
    
    ; Stubs are grouped in blocks with each block starting with a trampoline
    ; jump to interrupt_entry. This allows the use of a short jump (to the
    ; trampoline) in each stub, which decreases the total size of the stubs,
    ; hopefully decreasing cache misses.
    %rep IDT_VECTOR_COUNT / PER_BLOCK + 1
        %assign trampoline_ivt ivt
        %define trampoline .trampoline %+ trampoline_ivt
        
        %if ivt < IDT_VECTOR_COUNT
            ; This is a jump target
            align 8
            
trampoline:
            jmp interrupt_entry
        %endif
        
        %rep PER_BLOCK
            %if ivt < IDT_VECTOR_COUNT
; set irq_jtable.irqxx label
.irq %+ ivt:
                ; Push a null DWORD in lieu of the error code for interrupts
                ; that do not have one (only some CPU exceptions have an error
                ; code). We do this to maintain a consistent stack frame layout.
                %if ! HAS_ERRCODE(ivt)
                    push byte NULL_ERRCODE 
                %endif
                
                ; This if statement is not technically necessary, but it
                ; prevents the assembler from warning that the operand is
                ; out of bound for vectors > 127.
                %if ivt < 128
                    push byte ivt
                %else
                    ; Operand is sign-extended, so some post-processing needs to
                    ; be done in interrupt_entry.
                    push byte ivt-256
                %endif
                
                jmp short trampoline
                
                %assign ivt ivt+1
            %endif
        %endrep
        
    %endrep

; ------------------------------------------------------------------------------
; Interrupt Vector Table (IDT)
; ------------------------------------------------------------------------------
; Here, we reserve enough space for the IDT in the .data section (64 bits per
; entry/descriptor) and we store the address of the matching jump table stub in
; each entry. The kernel initialization code will re-process this table later
; and create actual interrupt gate descriptors from these addresses.

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
