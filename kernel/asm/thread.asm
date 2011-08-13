%define VM_FLAG_PRESENT 1<<0

; ------------------------------------------------------------------------------
; FUNCTION: switch_thread
; C PROTOTYPE: int switch_thread(addr_t vstack, phys_addr_t pstack, unsigned int flags, pte_t *pte, int next);
; ------------------------------------------------------------------------------
; initial stack layout:
;	esp+40	next
;	esp+36	pte
;	esp+32	flags
;	esp+28	pstack (high dword)
;	esp+24	pstack (low dword)
;	esp+20	vstack
;	esp+16	return address
;	esp+12	stored ebx
;	esp+ 8	stored esi
;	esp+ 4	stored edi
;	esp+ 0	stored ebp
	global switch_thread
switch_thread:
	; we need to store the registers even if we are not using them because
	; we are about ot switch the stack
	push ebx
	push esi
	push edi
	push ebp
	
	; virtual address of stack
	mov edx, [esp+20]	; First param:  vstack
	
	; create page directory entry
	mov ebx, [esp+24]	; Second param: pstack (low dword)
	or  ebx, [esp+32]	; Third param:  flags
	or  ebx, VM_FLAG_PRESENT
	
	; page directory entry address
	mov edi, [esp+36]	; Fourth param: pte
	
	; return value of function, passes information about what to do next
	; across the call
	mov eax, [esp+40]	; Fifth param: next
	
	; switch stack
	mov [edi], ebx		; write page directory entry
	invlpg [edx]		; invalidate TLB entry
	
	pop ebp
	pop edi
	pop esi
	pop ebx
	
	ret
