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
    
    extern free_thread

; ------------------------------------------------------------------------------
; FUNCTION: thread_context_switch_stack
; C PROTOTYPE: void thread_context_switch_stack(
;               machine_thread_t *from_ctx,
;               machine_thread_t *to_ctx,
;               bool destroy_from);
; ------------------------------------------------------------------------------
    global thread_context_switch_stack:function (thread_context_switch_stack.end - thread_context_switch_stack)
thread_context_switch_stack:
    ; System V ABI calling convention: these four registers must be preserved
    ;
    ; We must store these registers whether we are actually using them here or
    ; not since we are about to switch to another thread that will.
    push ebp
    mov ebp, esp    ; setup frame pointer
    push ebx
    push esi
    push edi
    
    ; At this point, the stack looks like this:
    ;
    ; esp+28  destroy_from boolean (third function argument)
    ; esp+24  to thread context pointer (second function argument)
    ; esp+20  from thread context pointer (first function argument)
    ; esp+16  return address
    ; esp+12  ebp
    ; esp+ 8  ebx
    ; esp+ 4  esi
    ; esp+ 0  edi
    
    ; retrieve the from thread context argument
    mov ecx, [esp+20]   ; from thread context (first function argument)

    ; On the first thread context switch after boot, the kernel is using a
    ; temporary stack and the from/current thread context is NULL. Skip saving
    ; the current stack pointer in that case.
    or ecx, ecx
    jz .do_switch

    ; store the current stack pointer in the first member of the thread context
    ; structure
    mov [ecx], esp

.do_switch:
    ; read remaining arguments from stack before switching
    mov esi, [esp+24]   ; to thread context (second function argument)
    mov eax, [esp+28]   ; destroy_from boolean (third function argument)
    
    ; Load the saved stack pointer from the thread context to which we are
    ; switching (to thread). This is where we actually switch thread.
    mov esp, [esi]      ; saved stack pointer is the first member
    
    ; Restore the saved registers.
    ;
    ; We do this before calling free_thread(). Otherwise, the frame pointer
    ; still refers to the thread stack for the previous thread, i.e. the one
    ; we are potentially about to destroy, when free_thread() is called.
    ; This is a problem if e.g. we try to dump the call stack from free_thread()
    ; or one of its callees.
    pop edi
    pop esi
    pop ebx
    pop ebp

    ; Now that we switched stack, see if the caller requested the from thread
    ; context be destroyed.
    or eax, eax
    jz .skip_destroy
    
    ; destroy from thread context
    and ecx, THREAD_CONTEXT_MASK
    push ecx
    call free_thread
    
    ; cleanup free_thread() arguments from stack
    add esp, 4

.skip_destroy:
    ret

.end:
