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

#include <jinue/shared/asm/i686.h>

    bits 32

    extern pthread_exit
    extern __pthread_set_current
    
    section .text

; ------------------------------------------------------------------------------
; FUNCTION: __pthread_entry
; C PROTOTYPE: void __pthread_entry(void);
; ------------------------------------------------------------------------------
    global __pthread_entry:function (__pthread_entry.end - __pthread_entry)
__pthread_entry:
    ; Set up the frame pointer
    mov ebp, esp

    ; Set current thread in Thread-Local Storage (TLS). The address of the
    ; thread, which is the single argument to this function, is on the top of
    ; the stack on entry.
    call __pthread_set_current

    ; Pop and discard the argument to __pthread_set_current() now that we no
    ; longer need it, then pop the address of the thread function. Leave the
    ; argument to the thread function on the stack since this is where the it
    ; will expect it.
    pop eax     ; discard __pthread_set_current() argument
    pop eax     ; thread function address

    ; Call the thread function.
    call eax

    ; Remove the thread function argument from the stack now that it is no
    ; longer needed and replace it with the return value of the thread
    ; function, which will serve as the argument to pthread_exit().
    pop ebx
    push eax

    ; Exit the thread.
    call pthread_exit

.end:
