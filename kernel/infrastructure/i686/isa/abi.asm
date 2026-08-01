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

    bits 32

; ------------------------------------------------------------------------------
; FUNCTION: get_frameptr
; C PROTOTYPE: void *get_frameptr(void)
; ------------------------------------------------------------------------------
    global get_frameptr:function (get_frameptr.end - get_frameptr)
get_frameptr:
    mov eax, ebp
    
    ret
.end:

; ------------------------------------------------------------------------------
; FUNCTION: get_caller_frameptr
; C PROTOTYPE: void *get_caller_frameptr(void *fptr)
; ------------------------------------------------------------------------------
    global get_caller_frameptr:function (get_caller_frameptr.end - get_caller_frameptr)
get_caller_frameptr:
    mov eax, [esp+4]        ; First argument: fptr
    mov eax, [eax]          ; Frame pointer to return
    
    ret
.end:

; ------------------------------------------------------------------------------
; FUNCTION: get_return_addr
; C PROTOTYPE: void *get_return_addr(void *fptr)
; ------------------------------------------------------------------------------
    global get_return_addr:function (get_return_addr.end - get_return_addr)
get_return_addr:
    mov eax, [esp+4]        ; First argument: fptr
    mov eax, [eax+4]        ; Return address to return

    ret
.end:

; ------------------------------------------------------------------------------
; FUNCTION: get_first_pointer_arg
; C PROTOTYPE: void *get_first_pointer_arg(void *fptr)
; ------------------------------------------------------------------------------
    global get_first_pointer_arg:function (get_first_pointer_arg.end - get_first_pointer_arg)
get_first_pointer_arg:
    mov eax, [esp+4]        ; First argument: fptr
    mov eax, [eax+8]        ; First argument on fptr frame

    ret
.end:
