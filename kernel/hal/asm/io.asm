    bits 32


; ------------------------------------------------------------------------------
; FUNCTION: inb
; C PROTOTYPE: uint8_t inb(uint16_t port)
; ------------------------------------------------------------------------------
    global inb
inb:
    mov edx, [esp+4]    ; first argument: port
    in al, dx
    movsx eax, al
    ret


; ------------------------------------------------------------------------------
; FUNCTION: inw
; C PROTOTYPE: uint16_t inw(uint16_t port)
; ------------------------------------------------------------------------------
    global inw
inw:
    mov edx, [esp+4]    ; first argument: port
    in ax, dx
    movsx eax, ax
    ret


; ------------------------------------------------------------------------------
; FUNCTION: inl
; C PROTOTYPE: uint32_t inl(uint16_t port)
; ------------------------------------------------------------------------------
    global inl
inl:
    mov edx, [esp+4]    ; first argument: port
    in eax, dx
    ret


; ------------------------------------------------------------------------------
; FUNCTION: outb
; C PROTOTYPE: void outb(uint16_t port, uint8_t value)
; ------------------------------------------------------------------------------
    global outb
outb:
    mov eax, [esp+8]    ; second argument: value
    mov edx, [esp+4]    ; first argument:  port
    out dx, al
    ret


; ------------------------------------------------------------------------------
; FUNCTION: outw
; C PROTOTYPE: void outw(uint16_t port, uint16_t value)
; ------------------------------------------------------------------------------
    global outw
outw:
    mov eax, [esp+8]    ; second argument: value
    mov edx, [esp+4]    ; first argument:  port
    out dx, ax
    ret


; ------------------------------------------------------------------------------
; FUNCTION: outl
; C PROTOTYPE: void outl(uint16_t port, uint32_t value)
; ------------------------------------------------------------------------------
    global outl
outl:
    mov eax, [esp+8]    ; second argument: value
    mov edx, [esp+4]    ; first argument:  port
    out dx, eax
    ret

