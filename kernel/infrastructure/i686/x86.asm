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

#include <kernel/infrastructure/i686/asm/x86.h>
#include <kernel/interface/i686/asm/boot.h>

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
; FUNCTION: invalidate_tlb
; C PROTOTYPE: void invalidate_tlb(void *vaddr)
; ------------------------------------------------------------------------------
    global invalidate_tlb:function (invalidate_tlb.end - invalidate_tlb)
invalidate_tlb:
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
; FUNCTION: cpuid
; C PROTOTYPE: uint32_t cpuid(x86_regs_t *regs)
; ------------------------------------------------------------------------------
    global cpuid:function (cpuid.end - cpuid)
cpuid:
    ; save registers
    push ebx
    push edi
    
    mov edi, [esp+12]   ; First param: regs
    
    mov eax, [edi+ 0]   ; regs->eax
    mov ebx, [edi+ 4]   ; regs->ebx
    mov ecx, [edi+ 8]   ; regs->ecx
    mov edx, [edi+12]   ; regs->edx
    
    cpuid
    
    mov edi, [esp+12]   ; First param: regs
    
    mov [edi+ 0], eax
    mov [edi+ 4], ebx
    mov [edi+ 8], ecx
    mov [edi+12], edx
    
    ; restore registers
    pop edi
    pop ebx
        
    ret

.end:

; ------------------------------------------------------------------------------
; FUNCTION: get_esp
; C PROTOTYPE: uint32_t get_esp(void)
; ------------------------------------------------------------------------------
    global get_esp:function (get_esp.end - get_esp)
get_esp:
    mov eax, esp
    ret

.end:

; ------------------------------------------------------------------------------
; FUNCTION: get_cr0
; C PROTOTYPE: uint32_t get_cr0(void)
; ------------------------------------------------------------------------------
    global get_cr0:function (get_cr0.end - get_cr0)
get_cr0:
    mov eax, cr0
    ret

.end:

; ------------------------------------------------------------------------------
; FUNCTION: get_cr2
; C PROTOTYPE: uint32_t get_cr2(void)
; ------------------------------------------------------------------------------
    global get_cr2:function (get_cr2.end - get_cr2)
get_cr2:
    mov eax, cr2
    ret

.end:

; ------------------------------------------------------------------------------
; FUNCTION: get_cr3
; C PROTOTYPE: uint32_t get_cr3(void)
; ------------------------------------------------------------------------------
    global get_cr3:function (get_cr3.end - get_cr3)
get_cr3:
    mov eax, cr3
    ret

.end:

; ------------------------------------------------------------------------------
; FUNCTION: get_cr4
; C PROTOTYPE: uint32_t get_cr4(void)
; ------------------------------------------------------------------------------
    global get_cr4:function (get_cr4.end - get_cr4)
get_cr4:
    mov eax, cr4
    ret

.end:

; ------------------------------------------------------------------------------
; FUNCTION: set_cr0
; C PROTOTYPE: void set_cr0(uint32_t val)
; ------------------------------------------------------------------------------
    global set_cr0:function (set_cr0.end - set_cr0)
set_cr0:
    mov eax, [esp+4]    ; First param: val
    mov cr0, eax
    
    jmp .do_ret         ; jump to flush the instruction queue
.do_ret:
    ret

.end:

; ------------------------------------------------------------------------------
; FUNCTION: set_cr3
; C PROTOTYPE: void set_cr3(uint32_t val)
; ------------------------------------------------------------------------------
    global set_cr3:function (set_cr3.end - set_cr3)
set_cr3:
    mov eax, [esp+4]    ; First param: val
    mov cr3, eax
    ret

.end:

; ------------------------------------------------------------------------------
; FUNCTION: set_cr4
; C PROTOTYPE: void set_cr4(uint32_t val)
; ------------------------------------------------------------------------------
    global set_cr4:function (set_cr4.end - set_cr4)
set_cr4:
    mov eax, [esp+4]    ; First param: val
    mov cr4, eax
    ret

.end:

; ------------------------------------------------------------------------------
; FUNCTION: get_eflags
; C PROTOTYPE: uint32_t get_eflags(void)
; ------------------------------------------------------------------------------
    global get_eflags:function (get_eflags.end - get_eflags)
get_eflags:
    pushfd
    pop eax
    ret

.end:

; ------------------------------------------------------------------------------
; FUNCTION: set_eflags
; C PROTOTYPE: void set_eflags(uint32_t val)
; ------------------------------------------------------------------------------
    global set_eflags:function (set_eflags.end - set_eflags)
set_eflags:
    mov eax, [esp+4]    ; First param: val
    push eax
    popfd
    ret

.end:

; ------------------------------------------------------------------------------
; FUNCTION: set_cs
; C PROTOTYPE: void set_cs(uint32_t val)
; ------------------------------------------------------------------------------
    global set_cs:function (set_cs.end - set_cs)
set_cs:
    mov eax, [esp+4]    ; First param: val
    pop edx             ; return address
    push eax
    push edx 
    retf

.end:

; ------------------------------------------------------------------------------
; FUNCTION: set_ds
; C PROTOTYPE: void set_ds(uint32_t val)
; ------------------------------------------------------------------------------
    global set_ds:function (set_ds.end - set_ds)
set_ds:
    mov eax, [esp+4]    ; First param: val
    mov ds, eax
    ret

.end:

; ------------------------------------------------------------------------------
; FUNCTION: set_es
; C PROTOTYPE: void set_es(uint32_t val)
; ------------------------------------------------------------------------------
    global set_es:function (set_es.end - set_es)
set_es:
    mov eax, [esp+4]    ; First param: val
    mov es, eax
    ret

.end:

; ------------------------------------------------------------------------------
; FUNCTION: set_fs
; C PROTOTYPE: void set_fs(uint32_t val)
; ------------------------------------------------------------------------------
    global set_fs:function (set_fs.end - set_fs)
set_fs:
    mov eax, [esp+4]    ; First param: val
    mov fs, eax
    ret

.end:

; ------------------------------------------------------------------------------
; FUNCTION: set_gs
; C PROTOTYPE: void set_gs(uint32_t val)
; ------------------------------------------------------------------------------
    global set_gs:function (set_gs.end - set_gs)
set_gs:
    mov eax, [esp+4]    ; First param: val
    mov gs, eax
    ret

.end:

; ------------------------------------------------------------------------------
; FUNCTION: set_ss
; C PROTOTYPE: void set_ss(uint32_t val)
; ------------------------------------------------------------------------------
    global set_ss:function (set_ss.end - set_ss)
set_ss:
    mov eax, [esp+4]    ; First param: val
    mov ss, eax
    ret

.end:

; ------------------------------------------------------------------------------
; FUNCTION: rdmsr
; C PROTOTYPE: uint64_t rdmsr(msr_addr_t addr)
; ------------------------------------------------------------------------------
    global rdmsr:function (rdmsr.end - rdmsr)
rdmsr:
    mov ecx, [esp+ 4]    ; First param:  addr
    
    rdmsr
    ret

.end:

; ------------------------------------------------------------------------------
; FUNCTION: wrmsr
; C PROTOTYPE: void wrmsr(msr_addr_t addr, uint64_t val)
; ------------------------------------------------------------------------------
    global wrmsr:function (wrmsr.end - wrmsr)
wrmsr:
    mov ecx, [esp+ 4]   ; First param:  addr
    mov eax, [esp+ 8]   ; Second param: val (low dword)
    mov edx, [esp+12]   ; Second param: val (high dword)
    
    wrmsr
    ret

.end:

; ------------------------------------------------------------------------------
; FUNCTION: get_gs_ptr
; C PROTOTYPE: uint32_t get_gs_ptr(uint32_t *ptr)
; ------------------------------------------------------------------------------
    global get_gs_ptr:function (get_gs_ptr.end - get_gs_ptr)
get_gs_ptr:
    mov ecx, [esp+ 4]   ; First param:  ptr
    mov eax, [gs:ecx]
    ret

.end:

; ------------------------------------------------------------------------------
; FUNCTION: rdtsc
; C PROTOTYPE: uint64_t rdtsc(void)
; ------------------------------------------------------------------------------
    global rdtsc:function (rdtsc.end - rdtsc)
rdtsc:
    rdtsc
    ret

.end:

; ------------------------------------------------------------------------------
; FUNCTION: x86_enable_pae
; C PROTOTYPE: void x86_enable_pae(uint32_t cr3_value)
; ------------------------------------------------------------------------------
    global x86_enable_pae:function (x86_enable_pae.end - x86_enable_pae)
x86_enable_pae:
    mov eax, [esp+ 4]   ; First argument: pdpt

    ; Jump to low-address alias
    jmp .just_here - BOOT_OFFSET_FROM_16MB
.just_here:

    ; Disable paging.
    mov ecx, cr0
    and ecx, ~X86_CR0_PG
    mov cr0, ecx

    ; Load CR3 with address of PDPT.
    mov cr3, eax

    ; Enable PAE.
    mov eax, cr4
    or eax, X86_CR4_PAE
    mov cr4, eax

    ; Re-enable paging (PG), prevent kernel from writing to read-only pages (WP).
    or ecx, X86_CR0_PG | X86_CR0_WP
    mov cr0, ecx

    ; No need to jump back to high alias. The ret instruction will take care
    ; of it because this is where the return address is.
    ret

.end:
