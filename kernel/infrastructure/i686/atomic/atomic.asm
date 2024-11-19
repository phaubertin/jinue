; Copyright (C) 2024 Philippe Aubertin.
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

; -----------------------------------------------------------------------------
; FUNCTION: add_atomic
; C PROTOTYPE: int add_atomic(int *value, int increment);
; -----------------------------------------------------------------------------
    global add_atomic:function (add_atomic.end - add_atomic)
add_atomic:
    mov ecx, [esp+8]            ; second argument: increment
    mov edx, [esp+4]            ; first argument: value

    mov eax, ecx                ; Copy increment in eax.
    lock xadd dword [edx], eax  ; Exchange and add.
    add eax, ecx                ; Add increment to return value.

    ret
.end:

; -----------------------------------------------------------------------------
; FUNCTION: or_atomic
; C PROTOTYPE: int or_atomic(int *value, int mask);
; -----------------------------------------------------------------------------
    global or_atomic:function (or_atomic.end - or_atomic)
or_atomic:
    mov edx, [esp+4]                ; first argument: pointer to value
    mov eax, dword [edx]            ; initial value in eax

.again:
    mov ecx, eax                    ; copy value
    or ecx, dword [esp+8]           ; second argument: mask

    ; Copy or result (ecx) into value ([edx]), but only if current value is the
    ; one we are expecting (eax). Otherwise, the curent value ([edx]) is copied
    ; into eax so we can try again.
    lock cmpxchg dword [edx], ecx

    jnz .again                      ; value changed, retry

    ret
.end:
