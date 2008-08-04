bits 32

global halt
global start
extern kernel
extern e820_map
extern boot_setup_addr

start:
	pop eax
	mov [boot_setup_addr], eax
	mov dword [e820_map], 0x7c00
	
	xor eax, eax
	push eax
	push eax		; End of call stack (useful for debugging)
	mov ebp, esp	; Initialize frame pointer
	
	call kernel

halt:
	cli
	hlt
	jmp halt

