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
; FUNCTION: init_spinlock
; C PROTOTYPE: void init_spinlock(spinlock_t *lock);
; -----------------------------------------------------------------------------
    global init_spinlock:function (init_spinlock.end - init_spinlock)
init_spinlock:
    mov eax, [esp+4]    ; first argument: lock
    mov dword [eax], 0  ; set lock value to zero

    ret
.end:

; -----------------------------------------------------------------------------
; FUNCTION: spin_lock
; C PROTOTYPE: void spin_lock(spinlock_t *lock);
; DESCRIPTION:
;   Simple ticket lock implementation. The whole lock is a double word (32
;   bits) with the low word (16 bits) being the "now serving" number and the
;   high word being the ticket number.
;   
;   For now, the assumption is that interrupts are disabled whenever we are in
;   the kernel, so there is no need to disable interrupts here.
; -----------------------------------------------------------------------------
    global spin_lock:function (spin_lock.end - spin_lock)
spin_lock:
    mov eax, [esp+4]            ; first argument: lock
    mov ecx, (1 << 16)          ; operand: add 1 to high word
    lock xadd dword [eax], ecx  ; Exchange and add to lock value.
                                ; Value before increment is now in ecx.

    shr ecx, 16                 ; Move ticket number to low word.
.loop:
    mov edx, [eax]              ; Get lock value.
    and edx, (1 << 16) - 1      ; Mask to keep only the low word.
    cmp edx, ecx                ; Compare with our ticket number.
    jz .done                    ; If it matches, we are done.

    pause                       ; Yes, this is a spinlock.
    jmp .loop                   ; Loop one more time.

.done:
    ret
.end:

; -----------------------------------------------------------------------------
; FUNCTION: spin_unlock
; C PROTOTYPE: void spin_unlock(spinlock_t *lock);
; -----------------------------------------------------------------------------
    global spin_unlock:function (spin_unlock.end - spin_unlock)
spin_unlock:
    mov eax, [esp+4]    ; first argument: lock
    inc word [eax]      ; Increase lower word to indicate completion.

    ret
.end:
