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
	mov eax, [esp+4]	; First param: vaddr
	invlpg [eax]
	ret

; ------------------------------------------------------------------------------
; FUNCTION: lgdt
; C PROTOTYPE: void lgdt(gdt_info_t *gdt_info)
; ------------------------------------------------------------------------------
	global lgdt
lgdt:
	mov eax, [esp+4]	; First param: vaddr
	add eax, 2          ; gdt_info_t structure contains two bytes of
	                    ; padding for alignment
	lgdt [eax]
	ret

; ------------------------------------------------------------------------------
; FUNCTION: cpuid
; C PROTOTYPE: unsigned long cpuid(x86_regs_t *regs)
; ------------------------------------------------------------------------------
	global cpuid
cpuid:
	mov edi, [esp+4]	; First param: regs
	
	mov eax, [edi]     ; regs->eax
	mov ebx, [edi+4]   ; regs->ebx
	mov ecx, [edi+8]   ; regs->ecx
	mov edx, [edi+12]  ; regs->edx
	
	cpuid
	
	mov edi, [esp+4]	; First param: regs
	
	mov [edi], eax
	mov [edi+4], ebx
	mov [edi+8], ecx
	mov [edi+12], edx		
	
	ret

; ------------------------------------------------------------------------------
; FUNCTION: get_cr0
; C PROTOTYPE: unsigned long get_cr0(void)
; ------------------------------------------------------------------------------
	global get_cr0
get_cr0:
	mov eax, cr0
	ret

; ------------------------------------------------------------------------------
; FUNCTION: get_cr1
; C PROTOTYPE: unsigned long get_cr1(void)
; ------------------------------------------------------------------------------
	global get_cr1
get_cr1:
	mov eax, cr1
	ret

; ------------------------------------------------------------------------------
; FUNCTION: get_cr2
; C PROTOTYPE: unsigned long get_cr2(void)
; ------------------------------------------------------------------------------
	global get_cr2
get_cr2:
	mov eax, cr2
	ret

; ------------------------------------------------------------------------------
; FUNCTION: get_cr3
; C PROTOTYPE: unsigned long get_cr3(void)
; ------------------------------------------------------------------------------
	global get_cr3
get_cr3:
	mov eax, cr3
	ret

; ------------------------------------------------------------------------------
; FUNCTION: get_cr4
; C PROTOTYPE: unsigned long get_cr4(void)
; ------------------------------------------------------------------------------
	global get_cr4
get_cr4:
	mov eax, cr4
	ret

; ------------------------------------------------------------------------------
; FUNCTION: set_cr0
; C PROTOTYPE: void set_cr0(unsigned long val)
; ------------------------------------------------------------------------------
	global set_cr0
set_cr0:
	mov eax, [esp+4]	; First param: val
	mov cr0, eax
	ret

; ------------------------------------------------------------------------------
; FUNCTION: set_cr0x
; C PROTOTYPE: void set_cr0x(unsigned long val)
; ------------------------------------------------------------------------------
	global set_cr0x
set_cr0x:
	mov eax, [esp+4]	; First param: val
	mov cr0, eax
	jmp do_ret			; jump to flush the instruction queue
do_ret:
	ret

; ------------------------------------------------------------------------------
; FUNCTION: set_cr1
; C PROTOTYPE: void set_cr1(unsigned long val)
; ------------------------------------------------------------------------------
	global set_cr1
set_cr1:
	mov eax, [esp+4]	; First param: val
	mov cr1, eax
	ret

; ------------------------------------------------------------------------------
; FUNCTION: set_cr2
; C PROTOTYPE: void set_cr2(unsigned long val)
; ------------------------------------------------------------------------------
	global set_cr2
set_cr2:
	mov eax, [esp+4]	; First param: val
	mov cr2, eax
	ret

; ------------------------------------------------------------------------------
; FUNCTION: set_cr3
; C PROTOTYPE: void set_cr3(unsigned long val)
; ------------------------------------------------------------------------------
	global set_cr3
set_cr3:
	mov eax, [esp+4]	; First param: val
	mov cr3, eax
	ret

; ------------------------------------------------------------------------------
; FUNCTION: set_cr4
; C PROTOTYPE: void set_cr4(unsigned long val)
; ------------------------------------------------------------------------------
	global set_cr4
set_cr4:
	mov eax, [esp+4]	; First param: val
	mov cr4, eax
	ret

; ------------------------------------------------------------------------------
; FUNCTION: set_cs
; C PROTOTYPE: void set_cs(unsigned long val)
; ------------------------------------------------------------------------------
	global set_cs
set_cs:
	mov eax, [esp+4]	; First param: val
	pop ebx             ; return address
	push eax
	push ebx 
	retf

; ------------------------------------------------------------------------------
; FUNCTION: set_ds
; C PROTOTYPE: void set_ds(unsigned long val)
; ------------------------------------------------------------------------------
	global set_ds
set_ds:
	mov eax, [esp+4]	; First param: val
	mov ds, eax
	ret

; ------------------------------------------------------------------------------
; FUNCTION: set_es
; C PROTOTYPE: void set_es(unsigned long val)
; ------------------------------------------------------------------------------
	global set_es
set_es:
	mov eax, [esp+4]	; First param: val
	mov es, eax
	ret

; ------------------------------------------------------------------------------
; FUNCTION: set_fs
; C PROTOTYPE: void set_fs(unsigned long val)
; ------------------------------------------------------------------------------
	global set_fs
set_fs:
	mov eax, [esp+4]	; First param: val
	mov fs, eax
	ret

; ------------------------------------------------------------------------------
; FUNCTION: set_gs
; C PROTOTYPE: void set_gs(unsigned long val)
; ------------------------------------------------------------------------------
	global set_gs
set_gs:
	mov eax, [esp+4]	; First param: val
	mov gs, eax
	ret

; ------------------------------------------------------------------------------
; FUNCTION: set_ss
; C PROTOTYPE: void set_ss(unsigned long val)
; ------------------------------------------------------------------------------
	global set_ss
set_ss:
	mov eax, [esp+4]	; First param: val
	mov ss, eax
	ret

; ------------------------------------------------------------------------------
; FUNCTION: set_data_segments
; C PROTOTYPE: void set_data_segments(unsigned long val)
; ------------------------------------------------------------------------------
	global set_data_segments
set_data_segments:
	mov eax, [esp+4]	; First param: val
	mov ds, eax
	mov es, eax
	mov fs, eax
	mov gs, eax
	ret