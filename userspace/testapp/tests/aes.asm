; Copyright (C) 2026 Philippe Aubertin.
; All rights reserved.
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions
; are met:
; 
; 1. Redistributions of source code must retain the above copyright
;    notice, this list of conditions and the following disclaimer.
; 
; 2. Redistributions in binary form must reproduce the above copyright
;    notice, this list of conditions and the following disclaimer in the
;    documentation and/or other materials provided with the distribution.
; 
; 3. Neither the name of the author nor the names of other contributors
;    may be used to endorse or promote products derived from this software
;    without specific prior written permission.
; 
; THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND
; ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
; WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
; DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY
; DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
; (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
; ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
; (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
; SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

; -----------------------------------------------------------------------------

    bits 32

    ; -------------------------------------------------------------------------
    ; Function: aes128_round_key
    ; C prototype:
    ;   void aes128_round_key(
    ;       const uint8_t   *current,
    ;       uint8_t         *next,
    ;       int              round)
    ; -------------------------------------------------------------------------
    global aes128_round_key:function (aes128_round_key.end - aes128_round_key)
aes128_round_key:
    mov eax, [esp + 4]  ; first argument: current round key
    movdqu xmm0, [eax]  ; load round key

    ; AESKEYGENASSIST instruction only accepts an immediate value as its third
    ; argument (RCON).
    mov eax, [esp + 12]  ; third argument: round number

    dec eax
    jnz .r2
    aeskeygenassist xmm1, xmm0, 0x01
    jmp short .done
.r2:
    dec eax
    jnz .r3
    aeskeygenassist xmm1, xmm0, 0x02
    jmp short .done
.r3:
    dec eax
    jnz .r4
    aeskeygenassist xmm1, xmm0, 0x04
    jmp short .done
.r4:
    dec eax
    jnz .r5
    aeskeygenassist xmm1, xmm0, 0x08
    jmp short .done
.r5:
    dec eax
    jnz .r6
    aeskeygenassist xmm1, xmm0, 0x10
    jmp short .done
.r6:
    dec eax
    jnz .r7
    aeskeygenassist xmm1, xmm0, 0x20
    jmp short .done
.r7:
    dec eax
    jnz .r8
    aeskeygenassist xmm1, xmm0, 0x40
    jmp short .done
.r8:
    dec eax
    jnz .r9
    aeskeygenassist xmm1, xmm0, 0x80
    jmp short .done
.r9:
    dec eax
    jnz .r10
    aeskeygenassist xmm1, xmm0, 0x1b
    jmp short .done
.r10:
    aeskeygenassist xmm1, xmm0, 0x36
.done:

    ; AESKEYGENASSIST places the target FIPS transformation into bits [127:96].
    ; We replicate this across all 4 32-bit words of xmm1.
    pshufd xmm1, xmm1, 0xff

    ; xmm2: [ W3 | W2 | W1 | W0 ]
    ; xmm0: [ W3 | W2 | W1 | W0 ]
    movdqa xmm2, xmm0

    ; xmm2: [ W2 | W1 | W0 | 0 ]
    pslldq xmm2, 4
    ; xmm0: [ W3 XOR W2 | W2 XOR W1 | W1 XOR W0 | W0 ]
    pxor xmm0, xmm2    

    ; xmm2: [ W1 | W0 | 0 | 0 ]
    pslldq xmm2, 4
    ; xmm0: [ W3 XOR W2 XOR W1 | W2 XOR W1 XOR W0 | W1 XOR W0 | W0 ]
    pxor xmm0, xmm2

    ; xmm2: [ W0 | 0 | 0 | 0 ]
    pslldq xmm2, 4
    ; xmm0: [ W3 XOR W2 XOR W1 XOR W0 | W2 XOR W1 XOR W0 | W1 XOR W0 | W0 ]
    pxor xmm0, xmm2    

    ; XOR the transformation (T) into all columns at once. By doing this at the
    ; very end, we avoid T cancelling itself during intermediate steps.
    pxor xmm0, xmm1

    mov eax, [esp + 8]  ; second argument: next round key (output)
    movdqu [eax], xmm0  ; save round key

    ret
.end:

    ; -------------------------------------------------------------------------
    ; Function: aes128_ctr_encrypt_block
    ; C prototype:
    ;   void aes128_ctr_encrypt_block(
    ;       const uint8_t   *counter,
    ;       const uint8_t   *round_keys,
    ;       const uint8_t   *plaintext,
    ;       uint8_t         *ciphertext)
    ; -------------------------------------------------------------------------
    global aes128_ctr_encrypt_block:function (aes128_ctr_encrypt_block.end - aes128_ctr_encrypt_block)
aes128_ctr_encrypt_block:
    mov eax, [esp + 4]  ; first argument: counter pointer
    movdqu xmm0, [eax]  ; load counter block

    mov eax, [esp + 8]  ; second argument: round_keys pointer
    movdqu xmm1, [eax]  ; load unaligned round key
    pxor xmm0, xmm1     ; initial whitening (XOR with Key 0)

    ; AES rounds
    movdqu xmm1, [eax + 1*16]   ; round 1
    aesenc xmm0, xmm1
    movdqu xmm1, [eax + 2*16]   ; round 2
    aesenc xmm0, xmm1
    movdqu xmm1, [eax + 3*16]   ; round 3
    aesenc xmm0, xmm1
    movdqu xmm1, [eax + 4*16]   ; round 4
    aesenc xmm0, xmm1
    movdqu xmm1, [eax + 5*16]   ; round 5
    aesenc xmm0, xmm1
    movdqu xmm1, [eax + 6*16]   ; round 6
    aesenc xmm0, xmm1
    movdqu xmm1, [eax + 7*16]   ; round 7
    aesenc xmm0, xmm1
    movdqu xmm1, [eax + 8*16]   ; round 8
    aesenc xmm0, xmm1
    movdqu xmm1, [eax + 9*16]   ; round 9
    aesenc xmm0, xmm1
    movdqu xmm1, [eax + 10*16]  ; round 10
    aesenclast xmm0, xmm1

    mov eax, [esp + 12] ; third argument: plaintext pointer
    movdqu xmm1, [eax]  ; load plaintext block
    pxor xmm0, xmm1     ; XOR encrypted counter with plaintext

    mov eax, [esp + 16] ; fourth argument: ciphertext pointer
    movdqu [eax], xmm0  ; store ciphertext result
    ret
.end:
