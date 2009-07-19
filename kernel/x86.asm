	bits 32

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

