bits 32

; ------------------------------------------------------------------------------
; FUNCTION: syscall_fast_intel
; C PROTOTYPE: int syscall_fast_intel(ipc_ref_t    dest,
;                                     unsigned int funct,
;                                     unsigned int arg1,
;                                     unsigned int arg2);
; ------------------------------------------------------------------------------

	global syscall_fast_intel
	global syscall_fast_intel.exit
syscall_fast_intel:
	mov ebx, [esp+ 4]	; First param:  dest
	mov eax, [esp+ 8]	; Second param: funct
	mov esi, [esp+12]	; Third param:  arg1
	mov edi, [esp+16]	; Fourth param: arg2

	sysenter

.exit:
	ret

; ------------------------------------------------------------------------------
; FUNCTION: syscall_fast_amd
; C PROTOTYPE: int syscall_fast_amd(ipc_ref_t    dest,
;                                   unsigned int funct,
;                                   unsigned int arg1,
;                                   unsigned int arg2);
; ------------------------------------------------------------------------------
	global syscall_fast_amd
	global syscall_fast_amd.exit
syscall_fast_amd:
	mov ebx, [esp+ 4]	; First param:  dest
	mov eax, [esp+ 8]	; Second param: funct
	mov esi, [esp+12]	; Third param:  arg1
	mov edi, [esp+16]	; Fourth param: arg2
	
	syscall

.exit:	
	ret

; ------------------------------------------------------------------------------
; FUNCTION: syscall_intr
; C PROTOTYPE: int syscall_intr(ipc_ref_t    dest,
;                               unsigned int funct,
;                               unsigned int arg1,
;                               unsigned int arg2);
; ------------------------------------------------------------------------------
	global syscall_intr
	global syscall_intr.exit
syscall_intr:
	mov ebx, [esp+ 4]	; First param:  dest
	mov eax, [esp+ 8]	; Second param: funct
	mov esi, [esp+12]	; Third param:  arg1
	mov edi, [esp+16]	; Fourth param: arg2
	
	int 0x80

.exit:	
	ret
