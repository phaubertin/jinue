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

#ifndef JINUE_KERNEL_INFRASTRUCTURE_I686_ASM_SERIAL_H_
#define JINUE_KERNEL_INFRASTRUCTURE_I686_ASM_SERIAL_H_

#define SERIAL_COM1_IOPORT  0x3f8

#define SERIAL_COM2_IOPORT  0x2f8

#define SERIAL_COM3_IOPORT  0x3e8

#define SERIAL_COM4_IOPORT  0x2e8


#define SERIAL_REG_DATA_BUFFER      0

#define SERIAL_REG_DIVISOR_LOW      0

#define SERIAL_REG_INTR_ENABLE      1

#define SERIAL_REG_DIVISOR_HIGH     1

#define SERIAL_REG_INTR_ID          2

#define SERIAL_REG_FIFO_CTRL        2

#define SERIAL_REG_LINE_CTRL        3

#define SERIAL_REG_MODEM_CTRL       4

#define SERIAL_REG_LINE_STATUS      5

#define SERIAL_REG_MODEM_STATUS     6

#define SERIAL_REG_SCRATCH          7


#define SERIAL_DEFAULT_IOPORT       SERIAL_COM1_IOPORT

#define SERIAL_MAX_IOPORT           (0x10000 - 8)

#define SERIAL_DEFAULT_BAUD_RATE    115200

#endif
