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

#include <kernel/infrastructure/i686/asm/descriptors.h>
#include <kernel/infrastructure/i686/asm/thread.h>

    bits 32
    
    extern sub_ref_to_object

; ------------------------------------------------------------------------------
; FUNCTION: switch_thread_stack
; C PROTOTYPE: void switch_thread_stack(
;               machine_thread_t    *from,
;               machine_thread_t    *to);
; ------------------------------------------------------------------------------
    global switch_thread_stack:function (switch_thread_stack.end - switch_thread_stack)
switch_thread_stack:
    ; System V ABI calling convention: these four registers must be preserved
    ;
    ; We must store these registers whether we are actually using them here or
    ; not since we are about to switch to another thread that will.
    push ebp
    mov ebp, esp    ; setup frame pointer
    push ebx
    push esi
    push edi
    push dword 0
    push dword 0
    
    ; At this point, the stack looks like this:
    ;
    ; esp+32  to thread context pointer (second function argument)
    ; esp+28  from thread context pointer (first function argument)
    ; esp+24  return address
    ; esp+20  ebp
    ; esp+16  ebx
    ; esp+12  esi
    ; esp+ 8  edi
    ; esp+ 4  space reserved for cleanup handler argument
    ; esp+ 0  space reserved for cleanup handler
    
    ; retrieve the from thread context argument
    mov ecx, [esp+28]   ; from thread context (first function argument)

    ; On the first thread context switch after boot, the kernel is using a
    ; temporary stack and the from/current thread context is NULL. Skip saving
    ; the current stack pointer in that case.
    or ecx, ecx
    jz .do_switch

    ; store the current stack pointer in the first member of the thread context
    ; structure
    mov [ecx], esp

.do_switch:
    ; read remaining argument from stack before switching
    mov esi, [esp+32]   ; to thread context (second function argument)
    
    ; Load the saved stack pointer from the thread context to which we are
    ; switching (to thread). This is where we actually switch thread.
    mov esp, [esi]      ; saved stack pointer is the first member

    ; Call the cleanup handler, if any
    pop eax
    or eax, eax
    jz .no_handler

    call eax

.no_handler:
    ; Remove the cleanup handler argument from the stack
    pop edi
    
    ; Restore the saved registers.
    pop edi
    pop esi
    pop ebx
    pop ebp

    ret
.end:
