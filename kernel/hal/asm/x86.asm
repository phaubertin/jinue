    bits 32

; ------------------------------------------------------------------------------
; FUNCTION: cli
; C PROTOTYPE: void cli(void);
; ------------------------------------------------------------------------------
    global cli
cli:
    cli
    ret

; ------------------------------------------------------------------------------
; FUNCTION: sti
; C PROTOTYPE: void sti(void);
; ------------------------------------------------------------------------------
    global sti
sti:
    sti
    ret

; ------------------------------------------------------------------------------
; FUNCTION: invalidate_tlb
; C PROTOTYPE: void invalidate_tlb(addr_t vaddr)
; ------------------------------------------------------------------------------
    global invalidate_tlb
invalidate_tlb:
    mov eax, [esp+4]    ; First param: vaddr
    invlpg [eax]
    ret

; ------------------------------------------------------------------------------
; FUNCTION: lgdt
; C PROTOTYPE: void lgdt(x86_pseudo_descriptor_t *gdt_desc)
; ------------------------------------------------------------------------------
    global lgdt
lgdt:
    mov eax, [esp+4]    ; First param: gdt_info
    add eax, 2          ; gdt_info_t structure contains two bytes of
                        ; padding for alignment
    lgdt [eax]
    ret

; ------------------------------------------------------------------------------
; FUNCTION: lidt
; C PROTOTYPE: void lidt(x86_pseudo_descriptor_t *idt_desc)
; ------------------------------------------------------------------------------
    global lidt
lidt:
    mov eax, [esp+4]    ; First param: idt_info
    add eax, 2          ; idt_info_t structure contains two bytes of
                        ; padding for alignment
    lidt [eax]
    ret

; ------------------------------------------------------------------------------
; FUNCTION: ltr
; C PROTOTYPE: void ltr(seg_selector_t sel)
; ------------------------------------------------------------------------------
    global ltr
ltr:
    mov eax, [esp+4]    ; First param: sel
    ltr ax
    ret

; ------------------------------------------------------------------------------
; FUNCTION: cpuid
; C PROTOTYPE: uint32_t cpuid(x86_regs_t *regs)
; ------------------------------------------------------------------------------
    global cpuid
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

; ------------------------------------------------------------------------------
; FUNCTION: get_esp
; C PROTOTYPE: uint32_t get_esp(void)
; ------------------------------------------------------------------------------
    global get_esp
get_esp:
    mov eax, esp
    ret

; ------------------------------------------------------------------------------
; FUNCTION: get_cr0
; C PROTOTYPE: uint32_t get_cr0(void)
; ------------------------------------------------------------------------------
    global get_cr0
get_cr0:
    mov eax, cr0
    ret

; ------------------------------------------------------------------------------
; FUNCTION: get_cr1
; C PROTOTYPE: uint32_t get_cr1(void)
; ------------------------------------------------------------------------------
    global get_cr1
get_cr1:
    mov eax, cr1
    ret

; ------------------------------------------------------------------------------
; FUNCTION: get_cr2
; C PROTOTYPE: uint32_t get_cr2(void)
; ------------------------------------------------------------------------------
    global get_cr2
get_cr2:
    mov eax, cr2
    ret

; ------------------------------------------------------------------------------
; FUNCTION: get_cr3
; C PROTOTYPE: uint32_t get_cr3(void)
; ------------------------------------------------------------------------------
    global get_cr3
get_cr3:
    mov eax, cr3
    ret

; ------------------------------------------------------------------------------
; FUNCTION: get_cr4
; C PROTOTYPE: uint32_t get_cr4(void)
; ------------------------------------------------------------------------------
    global get_cr4
get_cr4:
    mov eax, cr4
    ret

; ------------------------------------------------------------------------------
; FUNCTION: set_cr0
; C PROTOTYPE: void set_cr0(uint32_t val)
; ------------------------------------------------------------------------------
    global set_cr0
set_cr0:
    mov eax, [esp+4]    ; First param: val
    mov cr0, eax
    
    jmp .do_ret         ; jump to flush the instruction queue
.do_ret:
    ret

; ------------------------------------------------------------------------------
; FUNCTION: set_cr1
; C PROTOTYPE: void set_cr1(uint32_t val)
; ------------------------------------------------------------------------------
    global set_cr1
set_cr1:
    mov eax, [esp+4]    ; First param: val
    mov cr1, eax
    ret

; ------------------------------------------------------------------------------
; FUNCTION: set_cr2
; C PROTOTYPE: void set_cr2(uint32_t val)
; ------------------------------------------------------------------------------
    global set_cr2
set_cr2:
    mov eax, [esp+4]    ; First param: val
    mov cr2, eax
    ret

; ------------------------------------------------------------------------------
; FUNCTION: set_cr3
; C PROTOTYPE: void set_cr3(uint32_t val)
; ------------------------------------------------------------------------------
    global set_cr3
set_cr3:
    mov eax, [esp+4]    ; First param: val
    mov cr3, eax
    ret

; ------------------------------------------------------------------------------
; FUNCTION: set_cr4
; C PROTOTYPE: void set_cr4(uint32_t val)
; ------------------------------------------------------------------------------
    global set_cr4
set_cr4:
    mov eax, [esp+4]    ; First param: val
    mov cr4, eax
    ret

; ------------------------------------------------------------------------------
; FUNCTION: get_eflags
; C PROTOTYPE: uint32_t get_eflags(void)
; ------------------------------------------------------------------------------
    global get_eflags
get_eflags:
    pushfd
    pop eax
    ret

; ------------------------------------------------------------------------------
; FUNCTION: set_eflags
; C PROTOTYPE: void set_eflags(uint32_t val)
; ------------------------------------------------------------------------------
    global set_eflags
set_eflags:
    mov eax, [esp+4]    ; First param: val
    push eax
    popfd
    ret

; ------------------------------------------------------------------------------
; FUNCTION: set_cs
; C PROTOTYPE: void set_cs(uint32_t val)
; ------------------------------------------------------------------------------
    global set_cs
set_cs:
    mov eax, [esp+4]    ; First param: val
    pop edx             ; return address
    push eax
    push edx 
    retf

; ------------------------------------------------------------------------------
; FUNCTION: set_ds
; C PROTOTYPE: void set_ds(uint32_t val)
; ------------------------------------------------------------------------------
    global set_ds
set_ds:
    mov eax, [esp+4]    ; First param: val
    mov ds, eax
    ret

; ------------------------------------------------------------------------------
; FUNCTION: set_es
; C PROTOTYPE: void set_es(uint32_t val)
; ------------------------------------------------------------------------------
    global set_es
set_es:
    mov eax, [esp+4]    ; First param: val
    mov es, eax
    ret

; ------------------------------------------------------------------------------
; FUNCTION: set_fs
; C PROTOTYPE: void set_fs(uint32_t val)
; ------------------------------------------------------------------------------
    global set_fs
set_fs:
    mov eax, [esp+4]    ; First param: val
    mov fs, eax
    ret

; ------------------------------------------------------------------------------
; FUNCTION: set_gs
; C PROTOTYPE: void set_gs(uint32_t val)
; ------------------------------------------------------------------------------
    global set_gs
set_gs:
    mov eax, [esp+4]    ; First param: val
    mov gs, eax
    ret

; ------------------------------------------------------------------------------
; FUNCTION: set_ss
; C PROTOTYPE: void set_ss(uint32_t val)
; ------------------------------------------------------------------------------
    global set_ss
set_ss:
    mov eax, [esp+4]    ; First param: val
    mov ss, eax
    ret

; ------------------------------------------------------------------------------
; FUNCTION: set_data_segments
; C PROTOTYPE: void set_data_segments(uint32_t val)
; ------------------------------------------------------------------------------
    global set_data_segments
set_data_segments:
    mov eax, [esp+4]    ; First param: val
    mov ds, eax
    mov es, eax
    mov fs, eax
    mov gs, eax
    ret

; ------------------------------------------------------------------------------
; FUNCTION: rdmsr
; C PROTOTYPE: uint64_t rdmsr(msr_addr_t addr)
; ------------------------------------------------------------------------------
    global rdmsr
rdmsr:
    mov ecx, [esp+ 4]    ; First param:  addr
    
    rdmsr
    ret

; ------------------------------------------------------------------------------
; FUNCTION: wrmsr
; C PROTOTYPE: void wrmsr(msr_addr_t addr, uint64_t val)
; ------------------------------------------------------------------------------
    global wrmsr
wrmsr:
    mov ecx, [esp+ 4]   ; First param:  addr
    mov eax, [esp+ 8]   ; Second param: val (low dword)
    mov edx, [esp+12]   ; Second param: val (high dword)
    
    wrmsr
    ret
; ------------------------------------------------------------------------------
; FUNCTION: get_gs_ptr
; C PROTOTYPE: uint32_t get_gs_ptr(uint32_t *ptr);
; ------------------------------------------------------------------------------
    global get_gs_ptr
get_gs_ptr:
    mov ecx, [esp+ 4]   ; First param:  ptr
    mov eax, [gs:ecx]
    ret
