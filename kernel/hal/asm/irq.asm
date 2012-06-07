%define NVECTORS			256
%define GDT_KERNEL_DATA		2

	bits 32

	extern dispatch_interrupt

; ------------------------------------------------------------------------------
; FUNCTION: irq_save_state
; DESCRIPTION : Save current thread state, call interrupt dispatching function,
;               then restore state and return from interrupt.
; ------------------------------------------------------------------------------
	global irq_save_state
irq_save_state:
	; We use the version of the push instruction with a byte operand in the
	; jump table because it is the shortest form of this instruction (2 bytes).
	; However, the byte operand is sign extended by the instruction, which is
	; obviously not what we want. Here, we mask the most significant bits of
	; the interrupt vector to make it zero-extended instead.
	and dword [esp], 0xff
	
	cld
	
	; save thread state on stack
	push gs   ; 40
	push fs   ; 36
	push es   ; 32
	push ds   ; 28
	push ebp  ; 24
	push ecx  ; 20
	push edi  ; 16
	push esi  ; 12
	push eax  ; 8
	push edx  ; 4
	push ebx  ; 0
	
	; set data segment
	mov eax, GDT_KERNEL_DATA * 8 + 0
	mov ds, ax
	mov es, ax
	
	; set per-cpu data segment (TSS)
	;
	; The entry which follows the TSS descriptor in the GDT is a data segment
	; which also points to the TSS (same base address and limit).
	str ax			; get selector for TSS descriptor
	add ax, 8		; next entry
	mov gs, ax		; load gs with data segment selector
	
	; set function parameters
	mov eax, [esp+44] ; get interrupt vector
	push esp          ; Second param: system call parameters (for slow system call method)
	push eax          ; First param:  Interrupt vector
	
	; call interrupt dispatching function
	call dispatch_interrupt
	
	; remove parameters from stack
	add esp, 8
	
	; restore thread state
	pop ebx
	pop edx
	pop eax
	pop esi
	pop edi
	pop ecx
	pop ebp
	pop ds
	pop es
	pop	fs
	pop	gs
	
	; discard interrupt vector
	add esp, 4
	
	; return from interrupt
	iret	

; ------------------------------------------------------------------------------
; Interrupt Jump Table (irq_jtable)
; ------------------------------------------------------------------------------
section .text
align 32

	global irq_jtable
irq_jtable:
	
	%assign ivt 0
	%rep NVECTORS / 7 + 1
		%assign stone_ivt ivt
		%define stone .jump %+ stone_ivt
stone:
		jmp irq_save_state
		
		%rep 7
			%if ivt < NVECTORS
				; set irq_jtable.irqxx label 
				.irq %+ ivt:
				
				; This if statement is not technically necessary, but it
				; prevents the assembler from warning us that the operand is
				; out of bound for all vectors > 127.
				%if ivt < 128
					push byte ivt
				%else
					push byte ivt-256
				%endif
				
				jmp short stone
				
				%assign ivt ivt+1
			%endif
		%endrep
		
	%endrep

; ------------------------------------------------------------------------------
; Interrupt Vector Table (IDT)
; ------------------------------------------------------------------------------
; Here, we reserve enough space for the IDT in the .data section (64 bits per
; entry/descriptor) and we store in each entry the address of the matching
; jump table stub. The kernel initialization code will re-process this table
; later and create actual interrupt gate descriptors.

section .data
align 32

	global idt
idt:
	%assign ivt 0
	%rep NVECTORS
		; set to irq_jtable.irqxx label 
		dd irq_jtable.irq %+ ivt
		dd 0
		%assign ivt ivt+1
	%endrep
