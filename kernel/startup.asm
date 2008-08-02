bits 32

global halt
global start
extern kernel

start:
	xor eax, eax
	push eax
	push eax		; End of call stack (useful for debugging)
	mov ebp, esp	; Initialize frame pointer
	
	call kernel

halt:
	cli
	hlt
	jmp halt

