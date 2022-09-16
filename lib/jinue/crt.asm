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

    bits 32
    
    extern main
    extern auxvp
    extern jinue_environ

; ------------------------------------------------------------------------------
; ELF binary entry point
; ------------------------------------------------------------------------------    
    global _start
_start:
    ; we are going up
    cld
    
    ; keep a copy of address of argc before we modify the stack pointer
    mov esi, esp
    
    ; set frame pointer and move stack pointer to the end of the stack frame
    mov ebp, esp
    sub esp, 12
    
    ; Store argc as first argument to main while keeping a copy in eax. 
    ; Register esi is incremented by this operation to point on argv.
    lodsd
    mov dword [ebp-12], eax
    
    ; Store argv as second argument to main
    mov dword [ebp-8], esi
    
    ; Skip whole argv array (eax entries) as well as the following NULL pointer
    lea esi, [esi + 4 * eax + 4]
    
    ; Store envp as third argument to main
    mov dword [ebp-4], esi
    mov dword [jinue_environ], esi
    
    ; Increment esi (by 4 each time) until we find the NULL pointer which marks
    ; the end of the environment variables. Since esi is post-incremented, it
    ; will point at the address past the NULL pointer at the end of the loop,
    ; which is the start of the auxiliary vectors.
    xor eax, eax
    mov edi, esi
    repnz scasd
    
    ; Set address of auxiliary vectors
    mov dword [auxvp], edi
    
    ; Now that all arguments are where they should be, call main()
    call main
    
    ; main() must never return
