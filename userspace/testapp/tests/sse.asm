; Copyright (C) 2026 Philippe Aubertin.
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

; -----------------------------------------------------------------------------

#include "asm/sse.h"

    bits 32

    extern jinue_yield_thread

    ; -------------------------------------------------------------------------
    ; Function: compute_sse
    ; C prototype: void compute_sse(const void *tbuffer, void *result)
    ; -------------------------------------------------------------------------
    global compute_sse:function (compute_sse.end - compute_sse)
compute_sse:
    mov eax, dword [esp+4]          ; first argument:  tbuffer
    mov ecx, SSE_TEST_PER_THREAD    ; initialize loop counter

    pxor xmm0, xmm0                 ; clear working regiter (xmm0)

.loop:
    mov edx, SSE_TEST_CHUNK_SIZE    ; inner loop counter

.inner_loop:
    pxor xmm0, oword [eax]          ; XOR memory item with working register
    add eax, 16                     ; increment pointer to next item
    dec edx                         ; decrement inner loop counter
    jnz .inner_loop                 ; loop

    ; Save caller-saved registers before calling jinue_yield_thread().
    push eax
    push ecx

    ; We want to yield every few items to test the kernel's FPU state save
    ; and restore and make sure each thread's state remains isolated.
    call jinue_yield_thread

    ; Restore registers.
    pop ecx
    pop eax

    sub ecx, SSE_TEST_CHUNK_SIZE
    jnz .loop

    mov eax, dword [esp+8]          ; second argument: result
    movdqa oword [eax], xmm0        ; store result
    ret
.end:
