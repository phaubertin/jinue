; Copyright (C) 2019-2024 Philippe Aubertin.
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
; FUNCTION: inb
; C PROTOTYPE: uint8_t inb(uint16_t port)
; ------------------------------------------------------------------------------
    global inb:function (inb.end - inb)
inb:
    mov edx, [esp+4]    ; first argument: port
    in al, dx
    movsx eax, al
    ret
.end:

; ------------------------------------------------------------------------------
; FUNCTION: inw
; C PROTOTYPE: uint16_t inw(uint16_t port)
; ------------------------------------------------------------------------------
    global inw:function (inw.end - inw)
inw:
    mov edx, [esp+4]    ; first argument: port
    in ax, dx
    movsx eax, ax
    ret
.end:

; ------------------------------------------------------------------------------
; FUNCTION: inl
; C PROTOTYPE: uint32_t inl(uint16_t port)
; ------------------------------------------------------------------------------
    global inl:function (inl.end - inl)
inl:
    mov edx, [esp+4]    ; first argument: port
    in eax, dx
    ret
.end:

; ------------------------------------------------------------------------------
; FUNCTION: outb
; C PROTOTYPE: void outb(uint16_t port, uint8_t value)
; ------------------------------------------------------------------------------
    global outb:function (outb.end - outb)
outb:
    mov eax, [esp+8]    ; second argument: value
    mov edx, [esp+4]    ; first argument:  port
    out dx, al
    ret
.end:

; ------------------------------------------------------------------------------
; FUNCTION: outw
; C PROTOTYPE: void outw(uint16_t port, uint16_t value)
; ------------------------------------------------------------------------------
    global outw:function (outw.end - outw)
outw:
    mov eax, [esp+8]    ; second argument: value
    mov edx, [esp+4]    ; first argument:  port
    out dx, ax
    ret
.end:

; ------------------------------------------------------------------------------
; FUNCTION: outl
; C PROTOTYPE: void outl(uint16_t port, uint32_t value)
; ------------------------------------------------------------------------------
    global outl:function (outl.end - outl)
outl:
    mov eax, [esp+8]    ; second argument: value
    mov edx, [esp+4]    ; first argument:  port
    out dx, eax
    ret
.end:
