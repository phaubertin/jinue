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

#include <jinue/utils.h>
#include <string.h>
#include "../utils.h"
#include "aes.h"

void run_aes_test(void) {
    if(! bool_getenv("RUN_TEST_AES")) {
        return;
    }

    /* This is test vector #1 from RFC 3686 "Encrypting 16 octets using AES-CTR
     * with 128-bit key". */
    const uint8_t *plaintext = (const uint8_t *)"Single block msg";
    const uint8_t key[] = {
        0xae, 0x68, 0x52, 0xf8,
        0x12, 0x10, 0x67, 0xcc,
        0x4b, 0xf7, 0xa5, 0x76,
        0x55, 0x77, 0xf3, 0x9e
    };
    const uint8_t counter[] = {
        0x00, 0x00, 0x00, 0x30,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01
    };
    const uint8_t expected[] = {
        0xe4, 0x09, 0x5d, 0x4f,
        0xb7, 0xa7, 0xb3, 0x79,
        0x2d, 0x61, 0x75, 0xa3,
        0x26, 0x13, 0x11, 0xb8
    };

    uint8_t round_keys[16 * 11];
    memcpy(round_keys, key, 16);

    for(int round = 1; round <= 10; ++round) {
        aes128_round_key(&round_keys[(round - 1) * 16], &round_keys[round * 16], round);
    }

    uint8_t ciphertext[16];

    aes128_ctr_encrypt_block(counter, round_keys, plaintext, ciphertext);

    int result = memcmp(ciphertext, expected, 16);
    jinue_info("AES test result: %s", result == 0 ? "PASS" : "FAIL");
}
