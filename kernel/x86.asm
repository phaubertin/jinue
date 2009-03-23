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

