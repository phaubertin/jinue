    bits 32

    
; ------------------------------------------------------------------------------
; FUNCTION: inb
; C PROTOTYPE: unsigned char inb(unsigned short int port)
; ------------------------------------------------------------------------------
    global inb
inb:
    mov edx, [esp+4]    ; First param: port
    in al, dx
    movsx eax, al
    ret
    

; ------------------------------------------------------------------------------
; FUNCTION: inw
; C PROTOTYPE: unsigned short int inw(unsigned short int port)
; ------------------------------------------------------------------------------    
    global inw    
inw:
    mov edx, [esp+4]    ; First param: port
    in ax, dx
    movsx eax, ax
    ret


; ------------------------------------------------------------------------------
; FUNCTION: inl
; C PROTOTYPE: unsigned int inl(unsigned short int port)
; ------------------------------------------------------------------------------
    global inl
inl:
    mov edx, [esp+4]    ; First param: port
    in eax, dx
    ret


; ------------------------------------------------------------------------------
; FUNCTION: outb
; C PROTOTYPE: void outb(unsigned short int port, unsigned char value)
; ------------------------------------------------------------------------------
    global outb
outb:
    mov eax, [esp+8]    ; Second param: value
    mov edx, [esp+4]    ; First param: port
    out dx, al
    ret
    

; ------------------------------------------------------------------------------
; FUNCTION: outw
; C PROTOTYPE: void outw(unsigned short int port, unsigned short int value)
; ------------------------------------------------------------------------------    
    global outw
outw:
    mov eax, [esp+8]    ; Second param: value
    mov edx, [esp+4]    ; First param: port
    out dx, ax
    ret

    
; ------------------------------------------------------------------------------
; FUNCTION: outl
; C PROTOTYPE: void outl(unsigned short int port, unsigned int value)
; ------------------------------------------------------------------------------
    global outl
outl:
    mov eax, [esp+8]    ; Second param: value
    mov edx, [esp+4]    ; First param: port
    out dx, eax
    ret

