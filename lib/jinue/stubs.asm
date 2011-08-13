	bits 32
	
	section .text	
; ------------------------------------------------------------------------------
; FUNCTION: syscall_fast_intel
; C PROTOTYPE: int syscall_fast_intel(ipc_ref_t    dest,
;                                     unsigned int method,
;                                     unsigned int funct,
;                                     unsigned int arg1,
;                                     unsigned int arg2);
; ------------------------------------------------------------------------------
	global syscall_fast_intel
syscall_fast_intel:
	push ebx
	push esi
	push edi
	push ebp
	
	mov ebx, [esp+20]	; First param:  dest
	mov edx, [esp+24]	; Second param: method
	mov eax, [esp+28]	; Third param:  funct
	mov esi, [esp+32]	; Fourth param: arg1
	mov edi, [esp+36]	; Fifth param:  arg2
	
	; save return address and stack pointer
	mov ecx, .return_here
	mov ebp, esp
		
	sysenter

.return_here:	
	or edi, edi			; On return, edi is set to the address of errno if
	jz .noerrno			; the kernel set an error number in ebx.
	
	mov [edi], ebx
	
.noerrno:	
	pop ebp
	pop edi
	pop esi
	pop ebx

	ret

; ------------------------------------------------------------------------------
; FUNCTION: syscall_fast_amd
; C PROTOTYPE: int syscall_fast_amd(ipc_ref_t    dest,
;                                   unsigned int method,
;                                   unsigned int funct,
;                                   unsigned int arg1,
;                                   unsigned int arg2);
; ------------------------------------------------------------------------------
	global syscall_fast_amd
syscall_fast_amd:
	push ebx
	push esi
	push edi
	push ebp
	
	mov ebx, [esp+20]	; First param:  dest
	mov edx, [esp+24]	; Second param: method
	mov eax, [esp+28]	; Third param:  funct
	mov esi, [esp+32]	; Fourth param: arg1
	mov edi, [esp+36]	; Fifth param:  arg2
	
	syscall
	
	or edi, edi			; On return, edi is set to the address of errno if
	jz .noerrno			; the kernel set an error number in ebx.
	
	mov [edi], ebx
	
.noerrno:	
	pop ebp
	pop edi
	pop esi
	pop ebx

	ret

; ------------------------------------------------------------------------------
; FUNCTION: syscall_intr
; C PROTOTYPE: int syscall_intr(ipc_ref_t    dest,
;                               unsigned int method,
;                               unsigned int funct,
;                               unsigned int arg1,
;                               unsigned int arg2);
; ------------------------------------------------------------------------------
	global syscall_intr
syscall_intr:
	push ebx
	push esi
	push edi
	push ebp
	
	mov ebx, [esp+20]	; First param:  dest
	mov edx, [esp+24]	; Second param: method
	mov eax, [esp+28]	; Third param:  funct
	mov esi, [esp+32]	; Fourth param: arg1
	mov edi, [esp+36]	; Fifth param:  arg2
	
	int 0x80
	
	or edi, edi			; On return, edi is set to the address of errno if
	jz .noerrno			; the kernel set an error number in ebx.
	
	mov [edi], ebx
	
.noerrno:	
	pop ebp
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

	global syscall_stub_names
syscall_stub_names:
	dd .str0
	dd .str1
	dd .str2
.str0:
	db "sysenter/sysexit (fast Intel)", 0
.str1:
	db "syscall/sysret (fast AMD)", 0
.str2:
	db "interrupt", 0

