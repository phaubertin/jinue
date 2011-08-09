	bits 32
	
	section .text	
; ------------------------------------------------------------------------------
; FUNCTION: syscall_fast_intel
; C PROTOTYPE: int syscall_fast_intel(ipc_ref_t    dest,
;                                     unsigned int funct,
;                                     unsigned int arg1,
;                                     unsigned int arg2);
; ------------------------------------------------------------------------------
	global syscall_fast_intel
syscall_fast_intel:
	push ebx
	push esi
	push edi	
	
	mov ebx, [esp+16]	; First param:  dest
	mov eax, [esp+20]	; Second param: funct
	mov esi, [esp+24]	; Third param:  arg1
	mov edi, [esp+28]	; Fourth param: arg2
	
	sysenter
	
	or esi, esi			; On return, esi is set to the address of errno if
	jz .noerrno			; the kernel set an error number in ebx.
	
	mov [esi], ebx
	
.noerrno:	
	pop edi
	pop esi
	pop ebx

	ret

; ------------------------------------------------------------------------------
; FUNCTION: syscall_fast_amd
; C PROTOTYPE: int syscall_fast_amd(ipc_ref_t    dest,
;                                   unsigned int funct,
;                                   unsigned int arg1,
;                                   unsigned int arg2);
; ------------------------------------------------------------------------------
	global syscall_fast_amd
syscall_fast_amd:
	push ebx
	push esi
	push edi	
	
	mov ebx, [esp+16]	; First param:  dest
	mov eax, [esp+20]	; Second param: funct
	mov esi, [esp+24]	; Third param:  arg1
	mov edi, [esp+28]	; Fourth param: arg2
	
	syscall
	
	or esi, esi			; On return, esi is set to the address of errno if
	jz .noerrno			; the kernel set an error number in ebx.
	
	mov [esi], ebx
	
.noerrno:
	pop edi
	pop esi
	pop ebx

	ret

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
	
	or esi, esi			; On return, esi is set to the address of errno if
	jz .noerrno			; the kernel set an error number in ebx.
	
	mov [esi], ebx
	
.noerrno:	
	pop edi
	pop esi
	pop ebx

	ret


; ------------------------------------------------------------------------------
; system call stubs table
; ------------------------------------------------------------------------------
	section .rodata

	global syscall_stubs
syscall_stubs:
	dd syscall_fast_intel
	dd syscall_fast_amd
	dd syscall_intr
