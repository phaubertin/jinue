/*
 * Copyright (C) 2019-2022 Philippe Aubertin.
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

#include <kernel/domain/services/logging.h>
#include <kernel/infrastructure/i686/asm/uart16550a.h>
#include <kernel/infrastructure/i686/io.h>
#include <kernel/infrastructure/i686/uart16550a.h>

static void printn(int loglevel, const char *message, size_t n);

static logger_t logger = {
    .log = printn
}; 

static int base_ioport;

void init_uart16550a(const config_t *config) {
    if(! config->machine.serial_enable) {
        return;
    }

    base_ioport             = config->machine.serial_ioport;
    unsigned int baud_rate  = config->machine.serial_baud_rate;
    unsigned int divisor    = 115200 / baud_rate;

    /* disable interrupts */
    outb(base_ioport + UART165550A_REG_INTR_ENABLE, 0);

    /* 8N1, enable DLAB to allow setting baud rate */
    outb(base_ioport + UART165550A_REG_LINE_CTRL, 0x83);

    /* set baud rate */
    outb(base_ioport + UART165550A_REG_DIVISOR_LOW, (divisor & 0xff));
    outb(base_ioport + UART165550A_REG_DIVISOR_HIGH, ((divisor >> 8) & 0xff));

    /* 8N1, disable DLAB */
    outb(base_ioport + UART165550A_REG_LINE_CTRL, 0x03);

    /* enable and clear FIFO
     *
     * Receive FIFO trigger level is not relevant for us as we are only
     * transmitting. */
    outb(base_ioport + UART165550A_REG_FIFO_CTRL, 0x07);

    /* assert DTR and RTS */
    outb(base_ioport + UART165550A_REG_MODEM_CTRL, 0x03);

    register_logger(&logger);
}

static void putc(char c) {
    /* wait for the UART to be ready to accept a new character */
    while( (inb(base_ioport + UART165550A_REG_LINE_STATUS) & 0x20) == 0) {}

    outb(base_ioport + UART165550A_REG_DATA_BUFFER, c);
}

static void printn(int loglevel, const char *message, size_t n) {
    for(int idx = 0; idx < n && message[idx] != '\0'; ++idx) {
        putc(message[idx]);
    }

    putc('\n');
}
