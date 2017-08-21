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

; ------------------------------------------------------------------------------
; FUNCTION: get_fpointer
; C PROTOTYPE: addr_t get_fpointer(void)
; ------------------------------------------------------------------------------
    global get_fpointer
get_fpointer:
    mov eax, ebp
    
    ret


; ------------------------------------------------------------------------------
; FUNCTION: get_caller_fpointer
; C PROTOTYPE: get_caller_fpointer(addr_t fptr)
; ------------------------------------------------------------------------------
    global get_caller_fpointer
get_caller_fpointer:
    push ebp                ; Save ebp
    
    mov ebp, [esp+8]        ; First argument: fptr
    mov eax, [ebp]          ; Frame pointer to return
    
    pop ebp                 ; Restore ebp
    ret


; ------------------------------------------------------------------------------
; FUNCTION: get_ret_addr
; C PROTOTYPE: addr_t get_ret_addr(addr_t fptr)
; ------------------------------------------------------------------------------
    global get_ret_addr
get_ret_addr:
    push ebp                ; Save ebp
    
    mov ebp, [esp+8]        ; First argument: fptr
    mov eax, [ebp+4]        ; Return address to return
    pop ebp                 ; Restore ebp
    
    ret


; ------------------------------------------------------------------------------
; FUNCTION: get_program_counter
; C PROTOTYPE: addr_t get_program_counter(void)
; ------------------------------------------------------------------------------
    global get_program_counter
get_program_counter:
    mov eax, [esp]
    
    ret
