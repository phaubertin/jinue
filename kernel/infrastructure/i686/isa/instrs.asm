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
; FUNCTION: cli
; C PROTOTYPE: void cli(void);
; ------------------------------------------------------------------------------
    global cli:function (cli.end - cli)
cli:
    cli
    ret
.end:

; ------------------------------------------------------------------------------
; FUNCTION: sti
; C PROTOTYPE: void sti(void);
; ------------------------------------------------------------------------------
    global sti:function (sti.end - sti)
sti:
    sti
    ret
.end:

; ------------------------------------------------------------------------------
; FUNCTION: hlt
; C PROTOTYPE: void hlt(void);
; ------------------------------------------------------------------------------
    global hlt:function (hlt.end - hlt)
hlt:
    hlt
    ret
.end:

; ------------------------------------------------------------------------------
; FUNCTION: invlpg
; C PROTOTYPE: void invlpg(void *vaddr)
; ------------------------------------------------------------------------------
    global invlpg:function (invlpg.end - invlpg)
invlpg:
    mov eax, [esp+4]    ; First param: vaddr
    invlpg [eax]
    ret
.end:

; ------------------------------------------------------------------------------
; FUNCTION: lgdt
; C PROTOTYPE: void lgdt(x86_pseudo_descriptor_t *gdt_desc)
; ------------------------------------------------------------------------------
    global lgdt:function (lgdt.end - lgdt)
lgdt:
    mov eax, [esp+4]    ; First param: gdt_info
    add eax, 2          ; gdt_info_t structure contains two bytes of
                        ; padding for alignment
    lgdt [eax]
    ret
.end:

; ------------------------------------------------------------------------------
; FUNCTION: lidt
; C PROTOTYPE: void lidt(x86_pseudo_descriptor_t *idt_desc)
; ------------------------------------------------------------------------------
    global lidt:function (lidt.end - lidt)
lidt:
    mov eax, [esp+4]    ; First param: idt_info
    add eax, 2          ; idt_info_t structure contains two bytes of
                        ; padding for alignment
    lidt [eax]
    ret
.end:

; ------------------------------------------------------------------------------
; FUNCTION: ltr
; C PROTOTYPE: void ltr(seg_selector_t sel)
; ------------------------------------------------------------------------------
    global ltr:function (ltr.end - ltr)
ltr:
    mov eax, [esp+4]    ; First param: sel
    ltr ax
    ret
.end:

; ------------------------------------------------------------------------------
; FUNCTION: rdmsr
; C PROTOTYPE: uint64_t rdmsr(uint32_t addr)
; ------------------------------------------------------------------------------
    global rdmsr:function (rdmsr.end - rdmsr)
rdmsr:
    mov ecx, [esp+ 4]    ; First param:  addr
    
    rdmsr
    ret
.end:

; ------------------------------------------------------------------------------
; FUNCTION: wrmsr
; C PROTOTYPE: void wrmsr(uint32_t addr, uint64_t val)
; ------------------------------------------------------------------------------
    global wrmsr:function (wrmsr.end - wrmsr)
wrmsr:
    mov ecx, [esp+ 4]   ; First param:  addr
    mov eax, [esp+ 8]   ; Second param: val (low dword)
    mov edx, [esp+12]   ; Second param: val (high dword)
    
    wrmsr
    ret
.end:
