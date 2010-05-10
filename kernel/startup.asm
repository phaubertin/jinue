bits 32

global halt
global start

extern kernel
extern e820_map
extern boot_data
extern boot_heap
extern kernel_size
extern kernel_top
extern kernel_region_top

start:
	; point esi and eax to the start of boot sector data
	; (setup code pushed its start address on stack)
	pop eax
	sub eax, 19
	mov esi, eax
	
	; figure out size of kernel image
	add eax, 7      ; field: sysize
	mov eax, dword [eax]
	shl eax, 4
	mov dword [kernel_size], eax
	
	; align to page boundary
	mov ebx, eax
	and ebx, 0xfff
	jz set_kernel_top ; already aligned
	add eax, 0x1000   ; page size
	and eax, ~0xfff
	
set_kernel_top:
	add eax, start
	mov dword [kernel_top], eax
	
	; --------------------------------------
	; setup boot-time heap and kernel stack
	; see doc/layout.txt
	; --------------------------------------
	
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

	; setup heap and stack:
	;  - heap size four times the size of the bios memory map
	;  - stack size 8kb
	mov eax, edi
	mov [boot_heap], eax
	
	sub eax, [kernel_top]
	shl eax, 2             ; size of heap
	add eax, [kernel_top]
	add eax, 8192          ; size of stack
	
	; align address on a page boundary
	mov ebx, eax
	and ebx, 0xfff
	jz switch_stack  ; already aligned
	add eax, 0x1000  ; page size
	and eax, ~0xfff
	
	; use new stack
switch_stack:
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

