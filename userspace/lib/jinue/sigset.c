/*
 * Copyright (C) 2026 Philippe Aubertin.
 * All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the author nor the names of other contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <jinue/jinue.h>
#include <stdbool.h>
#include <stdint.h>

static inline bool signo_is_invalid(int signo) {
    return signo < 1 || signo > 64;
}

static inline unsigned int signo_index(int signo) {
    return signo < 33 ? 0 : 1;
}

static inline uint32_t signo_mask(int signo) {
    if(signo > 32) {
        return (1 << (signo - 33));
    }

    return (1 << (signo - 1));
}

static void set_perrno_invalid(int *perrno) {
    if(perrno != NULL)  {
        *perrno = JINUE_EINVAL;
    }
}

int jinue_sigaddset(jinue_sigset_t *set, int signo, int *perrno) {
    if(signo_is_invalid(signo)) {
        set_perrno_invalid(perrno);
        return -1;
    }
    
    set->sa_sigbits[signo_index(signo)] |= signo_mask(signo);
    
    return 0;
}

int jinue_sigdelset(jinue_sigset_t *set, int signo, int *perrno) {
    if(signo_is_invalid(signo)) {
        set_perrno_invalid(perrno);
        return -1;
    }
    
    set->sa_sigbits[signo_index(signo)] &= ~signo_mask(signo);
    
    return 0;
}

void jinue_sigemptyset(jinue_sigset_t *set) {
    set->sa_sigbits[0] = 0;
    set->sa_sigbits[1] = 0;
    set->sa_sigbits[2] = 0;
    set->sa_sigbits[3] = 0;
}

void jinue_sigfillset(jinue_sigset_t *set) {
    set->sa_sigbits[0] = -1;
    set->sa_sigbits[1] = -1;
    set->sa_sigbits[2] = -1;
    set->sa_sigbits[3] = -1;
}

int jinue_sigismember(const jinue_sigset_t *set, int signo, int *perrno) {
    if(signo_is_invalid(signo)) {
        set_perrno_invalid(perrno);
        return -1;
    }
    
    return !!(set->sa_sigbits[signo_index(signo)] & signo_mask(signo));
}
