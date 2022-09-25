/*
 * Copyright (C) 2019 Philippe Aubertin.
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

#ifndef _JINUE_COMMON_ASM_IPC_H
#define _JINUE_COMMON_ASM_IPC_H

/*  +-----------------------+------------------------+---------------+
    |      buffer_size      |       data_size        |     n_desc    |  arg3
    +-----------------------+------------------------+---------------+
    31                    20 19                     8 7              0 */

/** number of bits reserved for the message buffer size and data size fields */
#define JINUE_SEND_SIZE_BITS            12

/** number of bits reserved for the number of message descriptors */
#define JINUE_SEND_N_DESC_BITS          8

/** maximum size of a message buffer and of the data inside that buffer */
#define JINUE_SEND_MAX_SIZE             (1 << (JINUE_SEND_SIZE_BITS - 1))

/** maximum number of descriptors inside a message */
#define JINUE_SEND_MAX_N_DESC           ((1 << JINUE_SEND_N_DESC_BITS) - 1)

/** mask to extract the message buffer or data size fields */
#define JINUE_SEND_SIZE_MASK            ((1 << JINUE_SEND_SIZE_BITS) - 1)

/** mask to extract the number of descriptors inside a message */
#define JINUE_SEND_N_DESC_MASK          JINUE_SEND_MAX_N_DESC

/** offset of buffer size within arg3 */
#define JINUE_SEND_BUFFER_SIZE_OFFSET   (JINUE_SEND_N_DESC_BITS + JINUE_SEND_SIZE_BITS)

/** offset of data size within arg3 */
#define JINUE_SEND_DATA_SIZE_OFFSET     JINUE_SEND_N_DESC_BITS

/** offset of number of descriptors within arg3 */
#define JINUE_SEND_N_DESC_OFFSET        0


#define JINUE_ARGS_PACK_BUFFER_SIZE(s)  ((s) << JINUE_SEND_BUFFER_SIZE_OFFSET)

#define JINUE_ARGS_PACK_DATA_SIZE(s)    ((s) << JINUE_SEND_DATA_SIZE_OFFSET)

#define JINUE_ARGS_PACK_N_DESC(n)       ((n) << JINUE_SEND_N_DESC_OFFSET)

#endif
