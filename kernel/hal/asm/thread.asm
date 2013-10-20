%define VM_FLAG_PRESENT 1<<0

%define PFADDR_SHIFT    4

%define GDT_USER_CODE 	3
%define GDT_USER_DATA 	4	

	bits 32
; ------------------------------------------------------------------------------
; FUNCTION: thread_start
; C PROTOTYPE: void thread_start(addr_t entry, addr_t user_stack)
; ------------------------------------------------------------------------------
	global thread_start
thread_start:
	pop eax						; Discard return address: we won't be going back
								; that way
	pop ebx						; First param:  entry
	pop ecx						; Second param: user_stack
	
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

; ------------------------------------------------------------------------------
; FUNCTION: thread_switch
; C PROTOTYPE: int thread_switch(addr_t vstack, pfaddr_t pstack, unsigned int flags, pte_t *pte, int next);
; ------------------------------------------------------------------------------
; stack layout:
;	esp+36	next
;	esp+32	pte
;	esp+28	flags
;	esp+24	pstack (low dword)
;	esp+20	vstack
;	esp+16	return address
;	esp+12	stored ebx
;	esp+ 8	stored esi
;	esp+ 4	stored edi
;	esp+ 0	stored ebp
	global thread_switch
thread_switch:
	; we need to store the registers even if we are not using them because
	; we are about to switch the stack
	push ebx
	push esi
	push edi
	push ebp
	
	; virtual address of stack
	mov edx, [esp+20]	    ; First param:  vstack
	
	; create page directory entry
	mov ebx, [esp+24]	    ; Second param: pstack
	shl ebx, PFADDR_SHIFT   ; convert pfaddr_t value to physical address
	or  ebx, [esp+28]	    ; Third param:  flags
	or  ebx, VM_FLAG_PRESENT
	
	; page directory entry address
	mov edi, [esp+32]	    ; Fourth param: pte
	
	; return value of function, passes information about what to do next
	; across the call
	mov eax, [esp+36]	    ; Fifth param: next
	
	; switch stack
	mov [edi], ebx		    ; write page directory entry
	invlpg [edx]		    ; invalidate TLB entry
	
	pop ebp
	pop edi
	pop esi
	pop ebx
	
	ret
