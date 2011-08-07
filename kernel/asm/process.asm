	bits 32

%define GDT_KERNEL_CODE	1
%define GDT_USER_CODE 	3
%define GDT_USER_DATA 	4	

; ------------------------------------------------------------------------------
; FUNCTION: process_start
; C PROTOTYPE: void process_start(addr_t entry, addr_t stack)
; ------------------------------------------------------------------------------
	global process_start
process_start:
	pop eax						; Discard return address: we won't be going back
								; that way
	pop ebx						; First param:  entry
	pop ecx						; Second param: stack
	
	; set data segment selectors
	mov eax, 8 * GDT_USER_DATA + 3
	mov ds, eax
	mov es, eax
	mov fs, eax
	mov gs, eax
	
	push eax					; stack segment (ss), rpl = 3
	push ecx					; stack pointer (esp)
	push 2						; flags register (eflags)
	push 8 * GDT_USER_CODE + 3	; code segment (cs), rpl/cpl = 3
	push ebx                    ; entry point
	
	iretd
