bits 32

global halt
global start

extern kernel
extern e820_map
extern boot_data
extern kernel_size
extern kernel_top
extern kernel_region_top

start:
	pop eax
	sub eax, 19
	mov esi, eax
	
	; figure out size of kernel image and align to page boundary
	add eax, 7      ; field: sysize
	mov eax, dword [eax]
	shl eax, 4
	mov dword [kernel_size], eax
	
	add eax, 0x1000 ; page size
	and eax, ~0xfff
	
	add eax, start
	mov dword [kernel_top], eax
	
	; copy boot sector data
	mov edi, eax
	mov dword [boot_data], eax
	mov ecx, 32
	rep movsb
	mov eax, edi
	
	; copy e820 map data
	mov dword [e820_map], eax
	mov edi, eax
	mov esi, 0x7c00

e820_loop:
	mov eax, edi
	
	mov ecx, 20    ; size of one entry
	rep movsb
	
	add eax, 8     ; size field, first word
	mov ecx, dword [eax]
	or  ecx, ecx
	jnz e820_loop
	
	add eax, 4     ; size field, second word
	mov ecx, dword [eax]
	or  ecx, ecx
	jz e820_end
	
	jmp e820_loop	
e820_end:

	; align address on a page boundary
	mov eax, edi
	add eax, 0x1000
	and eax, ~0xfff
	
	; setup new stack
	add eax, 8192    ; size of stack
	mov esp, eax
	mov dword [kernel_region_top], eax
	
	xor eax, eax
	push eax
	push eax		; End of call stack (useful for debugging)
	mov ebp, esp	; Initialize frame pointer
	
	call kernel

halt:
	cli
	hlt
	jmp halt

