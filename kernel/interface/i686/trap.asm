; Copyright (C) 2019-2026 Philippe Aubertin.
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

#include <jinue/shared/asm/i686.h>
#include <kernel/infrastructure/i686/asm/descriptors.h>
#include <kernel/infrastructure/i686/asm/percpu.h>
#include <kernel/infrastructure/i686/asm/tss.h>
#include <kernel/interface/i686/asm/exceptions.h>
#include <kernel/interface/i686/asm/idt.h>
#include <kernel/machine/asm/machine.h>

    bits 32

    extern handle_trap
    extern restore_fpu_state

; ------------------------------------------------------------------------------
; FUNCTION: interrupt_entry
; DESCRIPTION : Save current thread state, call interrupt dispatching function,
;               then restore state and return from interrupt.
; IMPORTANT NOTE:
;   machine_dump_call_stack() makes assumptions about the name of this function
;   so it can continue the call stack dump accross the interrupt if it comes
;   from the kernel.
; ------------------------------------------------------------------------------
    global interrupt_entry:function (return_from_interrupt.end - interrupt_entry)
interrupt_entry:
    cld
    
    ; Once everything is saved and after some reshuffling, the stack layout
    ; matches the trapframe_t structure definition. It looks like this:
    ;
    ; esp+68  user stack segment
    ; esp+64  user stack pointer
    ; esp+60  user EFLAGS
    ; esp+56  user code segment
    ; esp+52  user return address
    ; esp+48  error code
    ; esp+44  trap number (interrupt vector)
    ; esp+40 eax (message/system call argument 0)
    ; esp+36 ecx
    ; esp+32 edx
    ; esp+28 ebx (message/system call argument 1)
    ; esp+24 ebp
    ; esp+20 esi (message/system call argument 2)
    ; esp+16 edi (message/system call argument 3)
    ; esp+12 ds
    ; esp+ 8 es
    ; esp+ 4 fs
    ; esp+ 0 gs

    ; arg0 to arg3 are the system call arguments. The kernel modifies these
    ; when handling system calls to set return values.
    push eax    ; 40 arg0
    push ecx    ; 36
    push edx    ; 32
    push ebx    ; 28 arg1
    push ebp    ; 24
    push esi    ; 20 arg2
    push edi    ; 16 arg3
    push ds     ; 12
    push es     ; 8
    push fs     ; 4
    push gs     ; 0
    
    ; We use the version of the push instruction with a byte operand in the
    ; interrupt vector stubs because it is the shortest form of this instruction
    ; (2 bytes). However, the byte operand is sign extended by the instruction,
    ; which is obviously not what we want. Here, we mask the most significant
    ; bits of the interrupt vector to make it zero-extended instead.
    and dword [esp+44], 0xff
    
    ; Clear frame pointer.
    mov ebp, 0
    
    ; set data segment
    mov ecx, SEG_SELECTOR(GDT_KERNEL_DATA, RPL_KERNEL)
    mov ds, cx
    mov es, cx
    
    ; set per-cpu data segment
    mov eax, SEG_SELECTOR(GDT_PER_CPU_DATA, RPL_KERNEL)
    mov gs, ax
    
    ; set handle_trap() function argument
    push esp            ; First argument:  trapframe
    
    ; call interrupt dispatching function
    call handle_trap
    
    ; remove argument(s) from stack
    add esp, 4

    ; new threads start here
    global return_from_interrupt:function (return_from_interrupt.end - return_from_interrupt)
return_from_interrupt:

    ; Restore FPU/SSE state
    call restore_fpu_state
    
    pop gs                  ; 0
    pop fs                  ; 4
    pop es                  ; 8
    pop ds                  ; 12
    pop edi                 ; 16
    pop esi                 ; 20
    pop ebp                 ; 24
    pop ebx                 ; 28
    pop edx                 ; 32
    pop ecx                 ; 36
    pop eax                 ; 40
    add esp, 8              ; 44 skip trap number
                            ; 48 skip error code
    
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
    
    push byte SEG_SELECTOR(GDT_USER_DATA, RPL_USER)     ; 68
    push ebp                                            ; 64 user stack pointer
    pushf                                               ; 60
    push byte SEG_SELECTOR(GDT_USER_CODE, RPL_USER)     ; 56
    push ecx                                            ; 52 user return address
    
    mov ebp, 0              ; setup dummy frame pointer
    
    push byte 0             ; 48 error code (unused)
    
    ; 44 trap number
    ; 
    ; This trap number tells handle_trap() this is a system call.
    push dword JINUE_I686_SYSCALL_INTERRUPT  
    push eax                ; 40 arg0
    push byte 0             ; 36 ecx (caller-saved by System V ABI)
    push byte 0             ; 32 edx (caller-saved by System V ABI)
    push ebx                ; 28 arg1
    push byte 0             ; 24 ebp (caller-saved by kernel calling convention)
    push esi                ; 20 arg2
    push edi                ; 16 arg3
    push ds                 ; 12
    push es                 ; 8
    push fs                 ; 4
    push gs                 ; 0
    
    ; set data segment
    mov ecx, SEG_SELECTOR(GDT_KERNEL_DATA, RPL_KERNEL)
    mov ds, cx
    mov es, cx
    
    ; set per-cpu data segment
    mov eax, SEG_SELECTOR(GDT_PER_CPU_DATA, RPL_KERNEL)
    mov gs, ax
    
    ; set handle_trap() function argument
    push esp                ; First argument: trapframe
    
    call handle_trap
    
    ; cleanup handle_trap() argument
    add esp, 4

    ; Restore FPU/SSE state
    call restore_fpu_state
    
    pop gs                  ; 0
    pop fs                  ; 4
    pop es                  ; 8
    pop ds                  ; 12
    pop edi                 ; 16
    pop esi                 ; 20
    pop ebp                 ; 24
    pop ebx                 ; 28
    add esp, 8              ; 32 skip edx (used for stack pointer by SYSEXIT)
                            ; 36 skip ecx (used for return address by SYSEXIT)
    pop eax                 ; 40
    add esp, 8              ; 44 skip trap number
                            ; 48 skip error code
    pop edx                 ; 52 return address
    add esp, 4              ; 56 skip user code segment
    popf                    ; 60
    pop ecx                 ; 64 user stack pointer
    ; no action needed      ; 68 skip user stack segment
    
    ; When we saved EFLAGS, IF was already cleared, so we need to explicitly
    ; re-enable interrupts.
    ; 
    ; The sti instruction takes effect after the *next* instruction, so after
    ; sysexit here, which is what we want. For this reason, it must be the last
    ; instruction before sysexit.
    sti
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
    
    ; load kernel stack pointer from TSS
    mov esp, [gs:PERCPU_OFFSET_TSS + TSS_OFFSET_ESP0]
    
    ; For details on the stack layout, see comments in interrupt_entry above and
    ; the definition of the trapframe_t type.
    
    push byte SEG_SELECTOR(GDT_USER_DATA, RPL_USER)     ; 68
    push ebp                                            ; 64 user stack pointer
    pushf                                               ; 60
    push byte SEG_SELECTOR(GDT_USER_CODE, RPL_USER)     ; 56
    push ecx                                            ; 52 user return address
    
    mov ebp, 0              ; setup dummy frame pointer
    
    push byte 0             ; 48 error code (unused)
    
    ; 44 trap number
    ; 
    ; This trap number tells handle_trap() this is a system call.
    push dword JINUE_I686_SYSCALL_INTERRUPT  
    push eax                ; 40 arg0
    push byte 0             ; 36 ecx (caller-saved by System V ABI)
    push byte 0             ; 32 edx (caller-saved by System V ABI)
    push ebx                ; 28 arg1
    push byte 0             ; 24 ebp (caller-saved by kernel calling convention)
    push esi                ; 20 arg2
    push edi                ; 16 arg3
    push ds                 ; 12
    push es                 ; 8
    push fs                 ; 4
    push byte 0             ; 0 gs (caller-saved by kernel calling convention)
    
    ; set data segment
    mov ecx, SEG_SELECTOR(GDT_KERNEL_DATA, RPL_KERNEL)
    mov ds, cx
    mov es, cx
    
    ; set handle_trap() function argument
    push esp                ; First argument: trapframe
    
    call handle_trap
    
    ; cleanup handle_trap() argument
    add esp, 4

    ; Restore FPU/SSE state
    call restore_fpu_state
    
    pop gs                  ; 0
    pop fs                  ; 4
    pop es                  ; 8
    pop ds                  ; 12
    pop edi                 ; 16
    pop esi                 ; 20
    pop ebp                 ; 24
    pop ebx                 ; 28
    pop edx                 ; 32
    add esp, 4              ; 36 skip ecx (used for return address by SYSRET)
    pop eax                 ; 40
    add esp, 8              ; 44 skip trap number
                            ; 48 skip error code
    pop ecx                 ; 52 return address
    add esp, 4              ; 56 skip user code segment
    popf                    ; 60
    pop esp                 ; 64 user stack pointer
    ; no action needed      ; 68 skip user stack segment
    
    ; When we saved EFLAGS, IF was already cleared, so we need to explicitly
    ; re-enable interrupts.
    ; 
    ; The sti instruction takes effect after the *next* instruction, so after
    ; sysret here, which is what we want. For this reason, it must be the last
    ; instruction before sysret.
    sti
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
                %if ! EXCEPTION_HAS_ERRCODE(ivt)
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
