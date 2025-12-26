/*
 * Copyright (C) 2022-2024 Philippe Aubertin.
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

#include <kernel/domain/services/cmdline.h>
#include <kernel/domain/services/logging.h>
#include <kernel/infrastructure/i686/asm/serial.h>
#include <kernel/machine/cmdline.h>

#define CMDLINE_ERROR_INVALID_PAE               (1<<0)

#define CMDLINE_ERROR_INVALID_SERIAL_ENABLE     (1<<1)

#define CMDLINE_ERROR_INVALID_SERIAL_BAUD_RATE  (1<<2)

#define CMDLINE_ERROR_INVALID_SERIAL_IOPORT     (1<<3)

#define CMDLINE_ERROR_INVALID_SERIAL_DEV        (1<<4)

#define CMDLINE_ERROR_INVALID_VGA_ENABLE        (1<<5)

static int cmdline_errors;

typedef enum {
    CMDLINE_OPT_NAME_PAE,
    CMDLINE_OPT_NAME_SERIAL_ENABLE,
    CMDLINE_OPT_NAME_SERIAL_BAUD_RATE,
    CMDLINE_OPT_NAME_SERIAL_IOPORT,
    CMDLINE_OPT_NAME_SERIAL_DEV,
    CMDLINE_OPT_NAME_VGA_ENABLE,
} cmdline_opt_names_t;

static const cmdline_enum_def_t kernel_option_names[] = {
    {"pae",                 CMDLINE_OPT_NAME_PAE},
    {"serial_enable",       CMDLINE_OPT_NAME_SERIAL_ENABLE},
    {"serial_baud_rate",    CMDLINE_OPT_NAME_SERIAL_BAUD_RATE},
    {"serial_ioport",       CMDLINE_OPT_NAME_SERIAL_IOPORT},
    {"serial_dev",          CMDLINE_OPT_NAME_SERIAL_DEV},
    {"vga_enable",          CMDLINE_OPT_NAME_VGA_ENABLE},
    {NULL, 0}
};

static const cmdline_enum_def_t opt_pae_names[] = {
    {"auto",        CONFIG_PAE_AUTO},
    {"require",     CONFIG_PAE_REQUIRE},
    {NULL, 0}
};

static const cmdline_enum_def_t serial_ports[] = {
    {"0",           SERIAL_COM1_IOPORT},
    {"1",           SERIAL_COM2_IOPORT},
    {"2",           SERIAL_COM3_IOPORT},
    {"3",           SERIAL_COM4_IOPORT},

    {"ttyS0",       SERIAL_COM1_IOPORT},
    {"ttyS1",       SERIAL_COM2_IOPORT},
    {"ttyS2",       SERIAL_COM3_IOPORT},
    {"ttyS3",       SERIAL_COM4_IOPORT},

    {"/dev/ttyS0",  SERIAL_COM1_IOPORT},
    {"/dev/ttyS1",  SERIAL_COM2_IOPORT},
    {"/dev/ttyS2",  SERIAL_COM3_IOPORT},
    {"/dev/ttyS3",  SERIAL_COM4_IOPORT},

    {"com1",        SERIAL_COM1_IOPORT},
    {"com2",        SERIAL_COM2_IOPORT},
    {"com3",        SERIAL_COM3_IOPORT},
    {"com4",        SERIAL_COM4_IOPORT},

    {"COM1",        SERIAL_COM1_IOPORT},
    {"COM2",        SERIAL_COM2_IOPORT},
    {"COM3",        SERIAL_COM3_IOPORT},
    {"COM4",        SERIAL_COM4_IOPORT},
    {NULL, 0}
};

static const cmdline_enum_def_t serial_baud_rates[] = {
    {"300",         300},
    {"600",         600},
    {"1200",        1200},
    {"2400",        2400},
    {"4800",        4800},
    {"9600",        9600},
    {"14400",       14400},
    {"19200",       19200},
    {"38400",       38400},
    {"57600",       57600},
    {"115200",      115200},
    {NULL, 0}
};

void machine_cmdline_start_parsing(machine_config_t *config) {
    cmdline_errors = 0;
}

bool machine_cmdline_has_errors(void) {
    return cmdline_errors != 0;
}

void machine_cmdline_process_option(
        machine_config_t        *config,
        const cmdline_token_t   *option,
        const cmdline_token_t   *value) {

    int opt_name;
    int ivalue;

    if(! cmdline_match_enum(&opt_name, kernel_option_names, option)) {
        return;
    }

    switch(opt_name) {
    case CMDLINE_OPT_NAME_PAE:
        if(!cmdline_match_enum((int *)&(config->pae), opt_pae_names, value)) {
            cmdline_errors |= CMDLINE_ERROR_INVALID_PAE;
        }
        break;
    case CMDLINE_OPT_NAME_SERIAL_ENABLE:
        if(!cmdline_match_boolean(&(config->serial_enable), value)) {
            cmdline_errors |= CMDLINE_ERROR_INVALID_SERIAL_ENABLE;
        }
        break;
    case CMDLINE_OPT_NAME_SERIAL_BAUD_RATE:
        if(!cmdline_match_enum(&(config->serial_baud_rate), serial_baud_rates, value)) {
            cmdline_errors |= CMDLINE_ERROR_INVALID_SERIAL_BAUD_RATE;
        }
        break;
    case CMDLINE_OPT_NAME_SERIAL_IOPORT:
        if(cmdline_match_integer(&ivalue, value)) {
            if(ivalue > SERIAL_MAX_IOPORT) {
                cmdline_errors |= CMDLINE_ERROR_INVALID_SERIAL_IOPORT;
            }
            else {
                config->serial_ioport = ivalue;
            }
        }
        else {
            cmdline_errors |= CMDLINE_ERROR_INVALID_SERIAL_IOPORT;
        }
        break;
    case CMDLINE_OPT_NAME_SERIAL_DEV:
        if(!cmdline_match_enum(&(config->serial_ioport), serial_ports, value)) {
            cmdline_errors |= CMDLINE_ERROR_INVALID_SERIAL_DEV;
        }
        break;
    case CMDLINE_OPT_NAME_VGA_ENABLE:
        if(!cmdline_match_boolean(&(config->vga_enable), value)) {
            cmdline_errors |= CMDLINE_ERROR_INVALID_VGA_ENABLE;
        }
        break;
    }
}

void machine_cmdline_report_errors(void) {
    if(cmdline_errors & CMDLINE_ERROR_INVALID_PAE) {
        warn("  Invalid value for argument 'pae'");
    }

    if(cmdline_errors & CMDLINE_ERROR_INVALID_SERIAL_ENABLE) {
        warn("  Invalid value for argument 'serial_enable'");
    }

    if(cmdline_errors & CMDLINE_ERROR_INVALID_SERIAL_BAUD_RATE) {
        warn("  Invalid value for argument 'serial_baud_rate'");
    }

    if(cmdline_errors & CMDLINE_ERROR_INVALID_SERIAL_IOPORT) {
        warn("  Invalid value for argument 'serial_ioport'");
    }

    if(cmdline_errors & CMDLINE_ERROR_INVALID_SERIAL_DEV) {
        warn("  Invalid value for argument 'serial_dev'");
    }

    if(cmdline_errors & CMDLINE_ERROR_INVALID_VGA_ENABLE) {
        warn("  Invalid value for argument 'vga_enable'");
    }
}
