/*
 * Copyright (C) 2024 Philippe Aubertin.
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

#ifndef JINUE_KERNEL_INFRASTRUCTURE_I686_DRIVERS_ASM_PIT8253_H
#define JINUE_KERNEL_INFRASTRUCTURE_I686_DRIVERS_ASM_PIT8253_H

/* I/O addresses */

#define PIT8253_IO_BASE         0x40

#define PIT8253_IO_COUNTER0     (PIT8253_IO_BASE + 0)

#define PIT8253_IO_COUNTER1     (PIT8253_IO_BASE + 1)

#define PIT8253_IO_COUNTER2     (PIT8253_IO_BASE + 2)

#define PIT8253_IO_CW_REG       (PIT8253_IO_BASE + 3)

/* Individual flag definitions */

/** BCD (1) or binary (0) counter selection */
#define PIT8253_CW_BCD          (1<<0)

#define PIT8253_CW_M0           (1<<1)

#define PIT8253_CW_M1           (1<<2)

#define PIT8253_CW_M2           (1<<3)

#define PIT8253_CW_RL0          (1<<4)

#define PIT8253_CW_RL1          (1<<5)

#define PIT8253_CW_SC0          (1<<6)

#define PIT8253_CW_SC1          (1<<7)

/* Combined flags - select counter */

#define PIT8253_CW_COUNTER0     0

#define PIT8253_CW_COUNTER1     PIT8253_CW_SC0

#define PIT8253_CW_COUNTER2     PIT8253_CW_SC1

/* Combined flags - read/load */

#define PIT8253_CW_LOAD_LSB_MSB (PIT8253_CW_RL1 | PIT8253_CW_RL0)

/* Combined flags - mode */

/** Mode 0: interrupt on terminal count */
#define PIT8253_CW_MODE0        0

/** Mode 1: programmable one-shot */
#define PIT8253_CW_MODE1        PIT8253_CW_M0

/** Mode 2: rate generator */
#define PIT8253_CW_MODE2        PIT8253_CW_M1

/** Mode 3: square wave rate generator */
#define PIT8253_CW_MODE3        (PIT8253_CW_M1 | PIT8253_CW_M0)

/** Mode 4: software-triggered strobe */
#define PIT8253_CW_MODE4        PIT8253_CW_M2

/** Mode 5: hardware-triggered strobe */
#define PIT8253_CW_MODE5        (PIT8253_CW_M2 | PIT8253_CW_M0)

#endif
