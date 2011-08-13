%define GDT_KERNEL_DATA		2

	bits 32
	
	extern dispatch_syscall

; ------------------------------------------------------------------------------
; FUNCTION: fast_intel_entry
; ------------------------------------------------------------------------------
	global fast_intel_entry
fast_intel_entry:
	; save return address and user stack
	push ebp	; user stack
	push ecx	; return address
	
	; save data segment selectors
	push ds
	push es
	
	; set data segment
	mov ecx, GDT_KERNEL_DATA * 8 + 0
	mov ds, cx
	mov es, cx
	
	; set per-cpu data segment (TSS)
	;
	; The entry which follows the TSS descriptor in the GDT is a data segment
	; which also points to the TSS (same base address and limit). We use
	; otherwise unused entries in the TSS to store per-cpu data.
	str cx			; get selector for TSS descriptor
	add cx, 8		; next entry
	mov gs, cx		; load gs with data segment selector
	
	; ipc_params_t structure
	push edi	; 16 args.arg2
	push esi	; 12 args.arg1
	push eax	;  8 args.funct
	push edx	;  4 args.method
	push ebx	;  0 args.dest
	
	; push a pointer to the previous structure as parameter to
	; the dispatch_syscall function
	push esp
	
	call dispatch_syscall
	
	; setup return value
	mov ebx, [esp+4+0]	;  0 ret.errno
	mov eax, [esp+4+8]	;  8 ret.val
	mov edi, [esp+4+16]	; 16 ret.errno
	
	; cleanup stack
	add esp, 24
	
	; restore data segment selectors
	pop es
	pop ds
	
	; we don't want user space to have our per-cpu data segment selector
	mov ecx, 0
	mov gs, cx
	
	; restore return address and user stack
	pop edx		; return address
	pop ecx		; user stack
	
	sysexit

; ------------------------------------------------------------------------------
; FUNCTION: fast_amd_entry
; ------------------------------------------------------------------------------
	global fast_amd_entry
fast_amd_entry:
	; save user stack pointer temporarily in ebp
	mov ebp, esp
	
	; get kernel stack pointer from TSS
	;
	; The entry which follows the TSS descriptor in the GDT is a data segment
	; which also points to the TSS (same base address and limit).
	str sp			; get selector for TSS descriptor
	add sp, 8		; next entry
	mov gs, sp		; load gs with data segment selector
	mov esp, [gs:4]	; load kernel stack pointer from TSS
	
	; save return address and user stack
	push ebp	; user stack
	push ecx	; return address
	
	; save data segment selectors
	push ds
	push es
	
	; set data segment
	mov ecx, GDT_KERNEL_DATA * 8 + 0
	mov ds, cx
	mov es, cx
	
	; ipc_params_t structure
	push edi	; 16 args.arg2
	push esi	; 12 args.arg1
	push eax	;  8 args.funct
	push edx	;  4 args.method
	push ebx	;  0 args.dest
	
	; push a pointer to the previous structure as parameter to
	; the dispatch_syscall function
	push esp
	
	call dispatch_syscall

	; setup return value
	mov ebx, [esp+4+0]	;  0 ret.errno
	mov eax, [esp+4+8]	;  8 ret.val
	mov edi, [esp+4+16]	; 16 ret.errno
	
	; cleanup stack
	add esp, 24
	
	; restore data segment selectors
	pop es
	pop ds
	
	; we don't want user space to have our per-cpu data segment selector
	mov ecx, 0
	mov gs, cx
	
	; restore return address and user stack
	pop ecx		; return address
	pop esp		; user stack
	
	sysret
