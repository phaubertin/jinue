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

; TODO this file is not being compiled, is not maintained and calls a system
; call that does not exist. Actually implement the system call and then fix
; this code.

    global jinue_getcontext:function (jinue_getcontext.end - jinue_getcontext)
jinue_getcontext:
    ; setup frame pointer
    push ebp
    mov ebp, esp
    
    ; allocate space for system call arguments
    sub esp, 16
    
    ; Stack layout:
    ; esp+24    argument 1: ucontext
    ; esp+20    return address
    ; esp+16    frame pointer (ebp)
    ; esp+12    message argument 3
    ; esp+ 8    message argument 2
    ; esp+ 4    message argument 1
    ; esp+ 0    message argument 0
    
    mov ecx, [esp+24]
    mov [esp+ 0], SYS_GETCONTEXT
    mov [esp+ 4], 0
    mov [esp+ 8], ecx
    mov [esp+12], JINUE_ARGS_PACK_BUFFER_SIZE(SIZEOF_UCONTEXT)
    
    push esp
    
    call jinue_syscall
    
    pop ecx
    
    mov eax, [ecx+0]
    or eax, eax
    jnz .exit
    
    ; Save the caller-preserved registers. The system call stub restored them
    ; before returning to us as required by the ABI but did overwrite them
    ; before doing the system call.
    lea ecx, [ecx+MCONTEXT_OFFSET]
    mov [ecx+EBX], ebx
    mov [ecx+ESI], esi
    mov [ecx+EDI], edi
    
    ; Return value
    mov [ecx+EAX], eax
    
    ; At least one kernel entry point clobbers GS.
    mov edx, 0
    mov dx, gs
    mov [ecx+GS], edx
    
    ; We want the frame pointer (ebp), the return address and the stack pointer
    ; to have the values they had before this function was called.
    mov edx, [esp+16]
    mov [ecx+EBP], edx
    
    mov edx, [esp+20]
    mov [ecx+EIP], edx
    
    mov edx, esp
    add esp, 24
    mov [ecx+ESP], edx
    mov [ecx+UESP], edx
    
.exit:
    ret

.end:
