%include "boot.inc"

	times 	0x1ed-($-$$) db 0	

magic:			dd 0xcafef00d

; This starts at 0x1f1
setup_sects:	db SETUP_SIZE / 512	; from boot.inc
root_flags:		dw 1
sysize:			dd KERNEL_SIZE / 16	; from boot.inc
ram_size:		dw 0
vid_mode:		dw 0xffff
root_dev:		dw 0

signature:
	dw 0xaa55	
