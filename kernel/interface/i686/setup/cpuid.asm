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

; -----------------------------------------------------------------------------

#include <kernel/infrastructure/i686/asm/cyrix.h>
#include <kernel/infrastructure/i686/asm/eflags.h>

    bits 32

    ; -------------------------------------------------------------------------
    ; Function: detect_cpuid
    ; C prototype: bool detect_cpuid(void)
    ; -------------------------------------------------------------------------
    ; Detects whether the CPU supports the cpuid instruction.
    ; -------------------------------------------------------------------------
    global detect_cpuid:function (detect_cpuid.end - detect_cpuid)
detect_cpuid:
    push ebx

    ; attempt counter: first try
    xor ecx, ecx

.tryagain:
    ; Check cpuid instruction is supported
    ;
    ; We must be able to change the value of bit 21 (ID) of eflags
    pushfd                      ; save eflags for later restore
    pushfd                      ; get eflags
    mov edx, dword [esp]        ; remember original eflags in edx
    
    xor dword [esp], EFLAGS_ID  ; invert ID bit
    popfd                       ; set eflags

    pushfd                      ; get eflags again...
    pop eax                     ; ...in eax

    xor eax, edx                ; compare original and new eflags
    test eax, EFLAGS_ID         ; ID bit...
    jz .nochange                ; ...should have changed

    popfd                       ; restore flags to original

    ; Success: cpuid instruction is supported
    ; Set boolean true as return value and return.
    mov eax, 1
    jmp .ret

.nochange:
    ; We were not able to change the ID flag in EFLAGS. let's check if this was
    ; was the first or second attempt.
    or ecx, ecx
    jnz .nope

    inc ecx

    ; It does not look like the cpuid instruction is supported. However, this
    ; might be an old Cyrix processor where we need to set a bit in a MSR to
    ; enable the instruction.
    ;
    ; First, let's use the procedure described in Appendix B of the 5x86 CPU
    ; BIOS Writer’s Guide to check if it is a Cyrix CPU.
    xor eax, eax
    sahf            ; clear SF, ZF, AF, PF, and CF flags
    mov eax, 5      ; numerator
    mov ebx, 2      ; denominator
    div bl          ; divide 5 by 2

    ; The division above will not have changed the flags on a Cyrix processor
    ; but will on other processors. Let's check this.
    lahf
    cmp ah, 2
    jne .nope       ; not Cyrix, cpuid not supported

    ; It looks like we have an old Cyrix CPU, let's try to enable the cpuid
    ; instruction and then reattempt detection.
    ;
    ; First, let's set MAPEN in CCR3 so we can access CCR4.
    mov al, CYRIX_CONFREG_CCR3
    out CYRIX_CONF_INDEX_IOPORT, al
    
    in al, CYRIX_CONF_DATA_IOPORT
    and al, 0xf
    or al, 0x10
    mov ah, al

    mov al, CYRIX_CONFREG_CCR3
    out CYRIX_CONF_INDEX_IOPORT, al

    mov al, ah
    out CYRIX_CONF_DATA_IOPORT, al

    ; Now, let's set CPUID in CCR5
    mov al, CYRIX_CONFREG_CCR4
    out CYRIX_CONF_INDEX_IOPORT, al
    
    in al, CYRIX_CONF_DATA_IOPORT
    or al, 0x80
    mov ah, al

    mov al, CYRIX_CONFREG_CCR4
    out CYRIX_CONF_INDEX_IOPORT, al

    mov al, ah
    out CYRIX_CONF_DATA_IOPORT, al

    ; Now that the cpuid instruction was (hopefully) enabled, let's retry the
    ; detection.
    jmp .tryagain

.nope:
    ; Set boolean false as return value and return.
    xor eax, eax
.ret:
    pop ebx
    ret
.end:
