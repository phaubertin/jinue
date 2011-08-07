; ------------------------------------------------------------------------------
; FUNCTION: syscall_intr
; C PROTOTYPE: int syscall_intr(ipc_ref_t    dest,
;                               unsigned int funct,
;                               unsigned int arg1,
;                               unsigned int arg2);
; ------------------------------------------------------------------------------
	global syscall_intr
syscall_intr:
	push ebx
	push esi
	push edi	
	
	mov ebx, [esp+16]	; First param:  dest
	mov eax, [esp+20]	; Second param: funct
	mov esi, [esp+24]	; Third param:  arg1
	mov edi, [esp+28]	; Fourth param: arg2
	
	int 0x80
	
	pop edi
	pop esi
	pop ebx

	ret
