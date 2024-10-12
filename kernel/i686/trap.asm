; Copyright (C) 2019 Philippe Aubertin.
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

#include <jinue/shared/asm/machine.h>
#include <kernel/i686/asm/descriptors.h>
#include <kernel/i686/asm/irq.h>

    bits 32

    extern dispatch_interrupt
    extern dispatch_syscall

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
    ; esp+68  user stack segment
    ; esp+64  user stack pointer
    ; esp+60  user EFLAGS
    ; esp+56  user code segment
    ; esp+52  user return address
    ;
    ; esp+48  ebp (but the error code is here on entry, see below)
    ; esp+44  interrupt vector
    ;
    ; esp+40 error code
    ; esp+36 gs
    ; esp+32 fs
    ; esp+28 es
    ; esp+24 ds
    ; esp+20 ecx
    ; esp+16 edx
    ; esp+12  edi (message/system call argument 3)
    ; esp+ 8  esi (message/system call argument 2)
    ; esp+ 4  ebx (message/system call argument 1)
    ; esp+ 0  eax (message/system call argument 0)
    
    sub esp, 4  ; 40 reserve space for the error code

    push gs     ; 36
    push fs     ; 32
    push es     ; 28
    push ds     ; 24
    push ecx    ; 20
    push edx    ; 16
    
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
    and dword [esp+44], 0xff
    
    ; If we are handling a CPU exception with an error code, the CPU pushed
    ; that error code right after the return address. Otherwise, the interrupt
    ; vector stub pushed a dummy value there to keep the same stack layout.
    ;
    ; However, we want to put ebp there because, if we are handling an interrupt
    ; that happened in the kernel, ebp contains the frame pointer. This makes
    ; debugging easier because it allows us to include what the kernel was 
    ; doing when the interrupt was triggered in the backtrace.
    mov edx, [esp+48]   ; read error code
    mov [esp+40], edx   ; save it where we actually want it
    mov [esp+48], ebp   ; save ebp (frame pointer) right after the return address
    
    ; Clear frame pointer until we know for sure we were in the kernel when
    ; the interrupt occurred. If the CPU was executing user code at the time,
    ; ebp is neither trustworthy nor useful.
    mov ebp, 0
    
    ; Check to see if we were in the kernel before the interrupt by looking at
    ; the return address
    cmp dword [esp+52], KLIMIT
    jb .skip_fp
    
    ; set frame pointer
    lea ebp, [esp+48]
    
.skip_fp:
    ; set data segment
    mov ecx, SEG_SELECTOR(GDT_KERNEL_DATA, RPL_KERNEL)
    mov ds, cx
    mov es, cx
    
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
    global return_from_interrupt:function (return_from_interrupt.end - return_from_interrupt)
return_from_interrupt:
    
    pop eax                 ; 0
    pop ebx                 ; 4
    pop esi                 ; 8
    pop edi                 ; 12
    pop edx                 ; 16
    pop ecx                 ; 20
    pop ds                  ; 24
    pop es                  ; 28
    pop fs                  ; 32
    pop gs                  ; 36
    add esp, 8              ; 40 skip error code
                            ; 44 skip interrupt vector
    pop ebp                 ; 48
    
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
    
    push byte 0             ; 48 ebp (caller-saved by kernel calling convention)
    push byte 0             ; 44 interrupt vector (unused)
    push byte 0             ; 40 error code (unused)
    push gs                 ; 36
    push fs                 ; 32
    push es                 ; 28
    push ds                 ; 24
    push byte 0             ; 20 ecx (caller-saved by System V ABI)
    push byte 0             ; 16 edx (caller-saved by System V ABI)
    
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
    
    ; set dispatch_syscall() function argument
    ;
    ; The message arguments, a ponter to which dispatch_syscall() takes
    ; as argument are at the beginning of the trap frame, so we can just
    ; pass the address of the trap frame.
    push esp                ; First argument: message arguments
    
    call dispatch_syscall
    
    ; cleanup dispatch_syscall() argument
    add esp, 4
    
    pop eax                 ; 0
    pop ebx                 ; 4
    pop esi                 ; 8
    pop edi                 ; 12
    add esp, 8              ; 16 skip edx (used for stack pointer by SYSEXIT)
                            ; 20 skip ecx (used for return address by SYSEXIT)
    pop ds                  ; 24
    pop es                  ; 28
    pop fs                  ; 32
    pop gs                  ; 36
    add esp, 8              ; 40 skip error code
                            ; 44 skip interrupt vector
    pop ebp                 ; 48
    pop edx                 ; 52 return address
    add esp, 4              ; 56 skip user code segment
    popf                    ; 60
    pop ecx                 ; 64 user stack pointer
    ; no action needed      ; 68 skip user stack segment
    
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
    
    push byte SEG_SELECTOR(GDT_USER_DATA, RPL_USER)     ; 68
    push ebp                                            ; 64 user stack pointer
    pushf                                               ; 60
    push byte SEG_SELECTOR(GDT_USER_CODE, RPL_USER)     ; 56
    push ecx                                            ; 52 user return address
    
    mov ebp, 0              ; setup dummy frame pointer
    
    push byte 0             ; 48 ebp (caller-saved by kernel calling convention)
    push byte 0             ; 44 interrupt vector (unused)
    push byte 0             ; 40 error code (unused)
    push byte 0             ; 36 gs (caller-saved by kernel calling convention)
    push fs                 ; 32
    push es                 ; 28
    push ds                 ; 24
    push byte 0             ; 20 ecx (caller-saved by System V ABI)
    push byte 0             ; 16 edx (caller-saved by System V ABI)
    
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
    
    ; set dispatch_syscall() function argument
    ;
    ; The message arguments, a ponter to which dispatch_syscall() takes
    ; as argument are at the beginning of the trap frame, so we can just
    ; pass the address of the trap frame.
    push esp                ; First argument: message arguments
    
    call dispatch_syscall
    
    ; cleanup dispatch_syscall() argument
    add esp, 4
    
    pop eax                 ; 0
    pop ebx                 ; 4
    pop esi                 ; 8
    pop edi                 ; 12
    pop edx                 ; 16
    add esp, 4              ; 20 skip ecx (used for return address by SYSRET)
    pop ds                  ; 24
    pop es                  ; 28
    pop fs                  ; 32
    pop gs                  ; 36
    add esp, 8              ; 40 skip error code
                            ; 44 skip interrupt vector
    pop ebp                 ; 48
    pop ecx                 ; 52 return address
    add esp, 4              ; 56 skip user code segment
    popf                    ; 60
    pop esp                 ; 64 user stack pointer
    ; no action needed      ; 68 skip user stack segment
    
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
