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
