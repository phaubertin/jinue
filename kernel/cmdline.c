/*
 * Copyright (C) 2022 Philippe Aubertin.
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

#include <hal/serial.h>
#include <ctype.h>
#include <cmdline.h>
#include <console.h>
#include <panic.h>
#include <printk.h>

/** Maximum valid command line length
 *
 * Here, the limiting factor is the size of the user loader's stack since these
 * options will end up on its command line or its environment.
 *
 * TODO we need to upgrade to boot protocol 2.06+ before we can increase this. */
#define CMDLINE_MAX_VALID_LENGTH    255

/** Maximum command line length that cmdline_parse_options() will attempt to parse
 *
 * cmdline_parse_options() can really parse any length. The intent here is to
 * attempt to detect a missing NUL terminator. */
#define CMDLINE_MAX_PARSE_LENGTH    (1000 * 1000)

#define CMDLINE_ERROR_TOO_LONG                  (1<<0)

#define CMDLINE_ERROR_IS_NULL                   (1<<1)

#define CMDLINE_ERROR_INVALID_PAE               (1<<2)

#define CMDLINE_ERROR_INVALID_SERIAL_ENABLE     (1<<3)

#define CMDLINE_ERROR_INVALID_SERIAL_BAUD_RATE  (1<<4)

#define CMDLINE_ERROR_INVALID_SERIAL_IOPORT     (1<<5)

#define CMDLINE_ERROR_INVALID_SERIAL_DEV        (1<<6)

#define CMDLINE_ERROR_INVALID_VGA_ENABLE        (1<<7)

#define CMDLINE_ERROR_JUNK_AFTER_ENDQUOTE       (1<<8)

typedef enum {
    PARSE_STATE_START,
    PARSE_STATE_NAME,
    PARSE_STATE_EQUAL,
    PARSE_STATE_START_QUOTE,
    PARSE_STATE_VALUE,
    PARSE_STATE_QUOTED_VALUE,
    PARSE_STATE_END_QUOTE,
    PARSE_STATE_DASH1,
    PARSE_STATE_DASH2,
    PARSE_STATE_RECOVER,
} parse_state_t;

typedef struct {
    const char *start;
    size_t      length;
} token_t;

typedef struct {
    const char  *name;
    int          enum_value;
} enum_def_t;

typedef enum {
    CMDLINE_OPT_NAME_PAE,
    CMDLINE_OPT_NAME_SERIAL_ENABLE,
    CMDLINE_OPT_NAME_SERIAL_BAUD_RATE,
    CMDLINE_OPT_NAME_SERIAL_IOPORT,
    CMDLINE_OPT_NAME_SERIAL_DEV,
    CMDLINE_OPT_NAME_VGA_ENABLE,
} cmdline_opt_names_t;

static const enum_def_t opt_names[] = {
    {"pae",                 CMDLINE_OPT_NAME_PAE},
    {"serial_enable",       CMDLINE_OPT_NAME_SERIAL_ENABLE},
    {"serial_baud_rate",    CMDLINE_OPT_NAME_SERIAL_BAUD_RATE},
    {"serial_ioport",       CMDLINE_OPT_NAME_SERIAL_IOPORT},
    {"serial_dev",          CMDLINE_OPT_NAME_SERIAL_DEV},
    {"vga_enable",          CMDLINE_OPT_NAME_VGA_ENABLE},
    {NULL, 0}
};

static const enum_def_t opt_pae_names[] = {
    {"auto",        CMDLINE_OPT_PAE_AUTO},
    {"disable",     CMDLINE_OPT_PAE_DISABLE},
    {"require",     CMDLINE_OPT_PAE_REQUIRE},
    {NULL, 0}
};

static const enum_def_t serial_ports[] = {
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

static const enum_def_t serial_baud_rates[] = {
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

static const enum_def_t bool_names[] = {
    {"true",    true},
    {"yes",     true},
    {"enable",  true},
    {"1",       true},
    {"false",   false},
    {"no",      false},
    {"disable", false},
    {"0",       false},
    {NULL, 0}
};

static cmdline_opts_t cmdline_options = {
    .pae                = CMDLINE_OPT_PAE_AUTO,
    .serial_enable      = false,
    .serial_baud_rate   = SERIAL_DEFAULT_BAUD_RATE,
    .serial_ioport      = SERIAL_DEFAULT_IOPORT,
    .vga_enable         = true,
};

static int cmdline_errors;

/**
 * Get the kernel command line options parsed with cmdline_parse_options().
 *
 * If called before cmdline_parse_options(), the returned options structure
 * contains the defaults.
 *
 * @return kernel command line options
 *
 * */
const cmdline_opts_t *cmdline_get_options(void) {
    return &cmdline_options;
}

/**
 * Process command line parsing errors
 *
 * This function is to be called after calling cmdline_parse_options(). It
 * logs a message for any parsing error that occurred and then panics if any
 * parsing error was fatal.
 *
 * This two-step process is needed because some command line arguments influence
 * how logging is performed (enable VGA and/or serial logging, baud rate, etc.).
 * Because of this, even if some parsing errors are found, we want to do our
 * best to parse these options so the errors are logged in the right way.
 *
 * The kernel's main function (i.e. kmain()) is expected to do the following, in
 * this order:
 * - Call cmdline_parse_options() to parse command line option.
 * - Initialize the console, taking into account the relevant options.
 * - Call this function.
 *
 * */
void cmdline_process_errors(void) {
    if(cmdline_errors != 0) {
        printk("There are issues with the kernel command line:\n");
    }

    if(cmdline_errors & CMDLINE_ERROR_TOO_LONG) {
        printk("    Kernel command line is too long\n");
    }

    if(cmdline_errors & CMDLINE_ERROR_IS_NULL) {
        printk("    No kernel command line/command line is NULL\n");
    }

    if(cmdline_errors & CMDLINE_ERROR_INVALID_PAE) {
        printk("    Invalid value for argument 'pae'\n");
    }

    if(cmdline_errors & CMDLINE_ERROR_INVALID_SERIAL_ENABLE) {
        printk("    Invalid value for argument 'serial_enable'\n");
    }

    if(cmdline_errors & CMDLINE_ERROR_INVALID_SERIAL_BAUD_RATE) {
        printk("    Invalid value for argument 'serial_baud_rate'\n");
    }

    if(cmdline_errors & CMDLINE_ERROR_INVALID_SERIAL_IOPORT) {
        printk("    Invalid value for argument 'serial_ioport'\n");
    }

    if(cmdline_errors & CMDLINE_ERROR_INVALID_SERIAL_DEV) {
        printk("    Invalid value for argument 'serial_dev'\n");
    }

    if(cmdline_errors & CMDLINE_ERROR_INVALID_VGA_ENABLE) {
        printk("    Invalid value for argument 'vga_enable'\n");
    }

    if(cmdline_errors & CMDLINE_ERROR_JUNK_AFTER_ENDQUOTE) {
        printk("    Invalid character after closing quote, separator (e.g. space) expected\n");
    }

    if(cmdline_errors != 0) {
        panic("Invalid kernel command line");
    }
}

/**
 * Attempt to match an enum value
 *
 * @param value set to numeric value of the enum only if there is a match
 * @param def enum definition
 * @param token potential name representing an enum value
 * @return true if an enum value was successfully matched, false otherwise
 *
 * */
static bool match_enum(int *value, const enum_def_t *def, const token_t *token) {
    for(int def_index = 0; def[def_index].name != NULL; ++def_index) {
        int token_index;

        const enum_def_t *this_def = &def[def_index];

        /* Optimism, captain! */
        bool match = true;

        for(token_index = 0; token_index < token->length; ++token_index) {
            int name_c  = this_def->name[token_index];
            int token_c = token->start[token_index];

            /* Corner case: if the name from the enum definition is a prefix of
             * the token, once we reach the end of it, name_c will be the NUL
             * character and token_c will not. */
            if(name_c != token_c) {
                match = false;
                break;
            }
        }

        if(match) {
            /* Up to now, all characters from the token and the enum definition
             * have matched. If we are at the end of the name from the enum
             * definition, we have a match. (Otherwise the token is just a
             * prefix of it.)
             *
             * Corner case: both strings might be empty, which is why we don't
             * use name_c here. (Its scope could have been outside the loop
             * instead of token_index.) */
            if(this_def->name[token_index] == '\0') {
                *value = this_def->enum_value;
                return true;
            }
        }
    }

    /* No match */
    return false;
}

/**
 * Convert a decimal or hexadecimal digit to it's integer value
 *
 * For characters 0-9, values 0-9 are returned
 * For characters a-f, values 10-15 are returned
 * For characters A-F, values 10-15 are returned
 *
 * For any other character, behaviour is undefined. The digit character is
 * expected to have been validated (i.e. in the right range) before this
 * function is called.
 *
 * @param c decimal or hexadecimal digit character
 * @return integer value of digit
 *
 * */

/* This function works for decimal and hexadecimal digits. Passing anything else
 * results in undefined behaviour, i.e. input needs to be validated first. */
static int integer(int c) {
    int ivalue = c - '0';

    if(ivalue < 10 && ivalue >= 0) {
        return ivalue;
    }

    ivalue = c - 'a' + 10;

    /* Because, why not? */
    if(ivalue < 36 && ivalue >= 0) {
        return ivalue;
    }

    return c - 'A' + 10;
}

/**
 * Attempt to match an integer value
 *
 * Decimal integer and hexadecimal values are accepted. Negative values are
 * rejected. Maximum value for a decimal value is currently 999 999 999 for
 * easier implementation.
 *
 * @param ivalue set to integer value only if there is a match
 * @param token potential integer value
 * @return true if parsed successfully as an integer, false otherwise
 *
 * */
static bool match_integer(int *ivalue, const token_t *value) {
    if(value->length < 1) {
        return false;
    }

    int c = value->start[0];

    if(value->length == 1) {
        if(!isdigit(c)) {
            return false;
        }
        *ivalue = integer(c);
        return true;
    }

    if(c == 0) {
        c = value->start[1];

        if(c != 'x' || value->length < 3) {
            /* Leading zero and not an hexadecimal value */
            return false;
        }

        /* Hexadecimal value
         *
         * Overflow check: eight nibbles to encode a 32-bit value, plus two for
         * the 0x ,prefix. */
        if(value->length > 10) {
            return false;
        }

        int acc = 0;

        for(int idx = 2; idx < value->length; ++ idx) {
            c = value->start[idx];

            if(! isxdigit(c)) {
                return false;
            }

            acc = 16 * acc + integer(c);
        }

        *ivalue = acc;
        return true;
    }
    else {
        /* Decimal value
         *
         * Overflow check: 999 999 999 is the largest value that will fit in 32
         * bits that can be checked against by only looking at the length of the
         * string. This is a bit sloppy but we don't need values that big so
         * let's keep it simple. */
        if(value->length > 9) {
            return false;
        }

        int acc = 0;

        for(int idx = 0; idx < value->length; ++ idx) {
            c = value->start[idx];

            if(! isdigit(c)) {
                return false;
            }

            acc = 10 * acc + integer(c);
        }

        *ivalue = acc;
        return true;
    }
}

/**
 * Given a name-value pair, set the relevant command line option
 *
 * Unrecognized options are ignored.
 *
 * @param name token representing the option name
 * @param value token representing the option value
 *
 * */
void process_name_value_pair(const token_t *name, const token_t *value) {
    int opt_name;
    int opt_value;

    if(! match_enum(&opt_name, opt_names, name)) {
        /* Unknown option - ignore */
        return;
    }

    switch(opt_name) {
    case CMDLINE_OPT_NAME_PAE:
        if(match_enum(&opt_value, opt_pae_names, value)) {
            cmdline_options.pae = opt_value;
        }
        else {
            cmdline_errors |= CMDLINE_ERROR_INVALID_PAE;
        }
        break;
    case CMDLINE_OPT_NAME_SERIAL_ENABLE:
        if(match_enum(&opt_value, bool_names, value)) {
            cmdline_options.serial_enable = opt_value;
        }
        else {
            cmdline_errors |= CMDLINE_ERROR_INVALID_SERIAL_ENABLE;
        }
        break;
    case CMDLINE_OPT_NAME_SERIAL_BAUD_RATE:
        if(match_enum(&opt_value, serial_baud_rates, value)) {
            cmdline_options.serial_baud_rate = opt_value;
        }
        else {
            cmdline_errors |= CMDLINE_ERROR_INVALID_SERIAL_BAUD_RATE;
        }
        break;
    case CMDLINE_OPT_NAME_SERIAL_IOPORT:
        if(match_integer(&opt_value, value)) {
            if(opt_value > SERIAL_MAX_IOPORT) {
                cmdline_errors |= CMDLINE_ERROR_INVALID_SERIAL_IOPORT;
            }
            else {
                cmdline_options.serial_ioport = opt_value;
            }
        }
        else {
            cmdline_errors |= CMDLINE_ERROR_INVALID_SERIAL_IOPORT;
        }
        break;
    case CMDLINE_OPT_NAME_SERIAL_DEV:
        if(match_enum(&opt_value, serial_ports, value)) {
            cmdline_options.serial_ioport = opt_value;
        }
        else {
            cmdline_errors |= CMDLINE_ERROR_INVALID_SERIAL_DEV;
        }
        break;
    case CMDLINE_OPT_NAME_VGA_ENABLE:
        if(match_enum(&opt_value, bool_names, value)) {
            cmdline_options.vga_enable = opt_value;
        }
        else {
            cmdline_errors |= CMDLINE_ERROR_INVALID_VGA_ENABLE;
        }
        break;
    }
}

/**
 * Check whether a character represents a command line separator
 *
 * A space or horizontal tab is a separator.
 *
 * The terminating NUL is also treated as a "separator", which ensures the last
 * option on the command line is processed correctly. This is important in
 * parsing states that represent the end of an option. In other states, it
 * really does not matter because the end of the command line means parsing is
 * done either way. (See cmdline_parse_options() for this paragraph to make
 * sense.)
 *
 * @param c character to check
 * @return true if the character is a separator, false otherwise
 *
 * */
static bool is_separator(int c) {
    return c == ' ' || c == '\t' || c == '\0';
}

/**
 * Parse the kernel command line options
 *
 * After this function is called, the options can be retrieved by calling
 * cmdline_get_options().
 *
 * This function is fairly permissive. Unrecognized options or options without
 * and equal sign do not make the command line invalid. These options are
 * likely intended for the initial process rather than the kernel, so they are
 * ignored.
 *
 * Parsing may fail if options are malformed or it the command line is too long,
 * but we want to do our best to continue parsing other valid options when this
 * happens. An invalid command line will lead to a kernel panic, but some
 * options affect logging (VGA and/or serial port enabled, baud rate, etc.) and
 * we would rather log the failure at the right place in the right way if
 * possible.
 *
 * @param cmdline command line string
 *
 * */
void cmdline_parse_options(const char *cmdline) {
    token_t name;
    token_t value;

    /* Some command line arguments affect how e do logging (e.g. VGA and/or
     * serial port enabled, baud rate, etc.). For this reason, when we encounter
     * an error, we continue parsing the arguments. The kernel's main function
     * does things in this order:
     *
     * - Parse the command line with this function.
     * - Initialize the console (VGA and serial port) based on the command line
     *   options.
     * - Call cmdline_process_errors() to report command line parsing errors,
     *   ideally on the right logging devices if the relevant options weren't
     *   affected.
     *
     * The cmdline_errors variable keeps track of errors for use later by the
     * cmdline_process_errors() function. */
    cmdline_errors = 0;

    if(cmdline == NULL) {
        cmdline_errors |= CMDLINE_ERROR_IS_NULL;
        return;
    }

    parse_state_t state = PARSE_STATE_START;
    bool done           = false;
    int pos             = 0;

    while(true) {
        const char *current = &cmdline[pos];
        char c              = *current;

        /* TODO check how the 32-bit setup code behaves when it copies the command line. */
        if(pos >= CMDLINE_MAX_VALID_LENGTH) {
            /* Command line is too long. The limiting factor here is the stack
             * size of the user loader and of the initial process, since we intend
             * to copy the options from this command line to their command line
             * and environment.
             *
             * Let's still continue parsing though so we have better chances to
             * have the right VGA and serial port options when we report this
             * error. */
            cmdline_errors |= CMDLINE_ERROR_TOO_LONG;

            if(pos >= CMDLINE_MAX_PARSE_LENGTH) {
                /* The command line is *way* too long, probably because the
                 * terminating NUL character is missing. Let's give up. */
                break;
            }
        }

        switch(state) {
        case PARSE_STATE_START:
            if(c == '-') {
                /* We might be at the start of an option that starts with one or
                 * more dashes, or we might also be at the start of the double
                 * dash that marks the end of the options parsed by the kernel.
                 * We will only know for sure later. */
                name.start  = current;
                state       = PARSE_STATE_DASH1;
            }
            else if(! is_separator(c)) {
                /* We are at the start of an option, possibly a name-value pair,
                 * i.e. a name and value separated by an equal sign. */
                name.start  = current;
                state       = PARSE_STATE_NAME;
            }
            break;
        case PARSE_STATE_NAME:
            if(c == '=') {
                /* We just encountered an equal sign, so we are at the end of
                 * the name in what looks like an name-value pair. */
                name.length = current - name.start;
                state = PARSE_STATE_EQUAL;
            }
            else if(is_separator(c)) {
                /* We did not encounter an equal sign, so let's just ignore this
                 * option. */
                state = PARSE_STATE_START;
            }
            break;
        case PARSE_STATE_EQUAL:
            if(is_separator(c)) {
                /* The empty string is not valid for any option we currently
                 * support. */
                state = PARSE_STATE_START;
            }
            else if(c == '"') {
                /* Looks like this is going to be a value in quotes. The value
                 * will end with a closing quote, not with the next separator
                 * (i.e. space) or the end of the command line. */
                state = PARSE_STATE_START_QUOTE;
            }
            else {
                /* We are at the start of an unquoted value. */
                value.start = current;
                state = PARSE_STATE_VALUE;
            }
            break;
        case PARSE_STATE_START_QUOTE:
            /* We are at the start of a value in quotes. This state is needed so
             * the value excludes the opening quote. */
            value.start = current;
            state = PARSE_STATE_QUOTED_VALUE;
            break;
        case PARSE_STATE_VALUE:
            if(is_separator(c)) {
                /* We are at the end of a name-value pair. Time to process it. */
                value.length = current - value.start;
                process_name_value_pair(&name, &value);
                state = PARSE_STATE_START;
            }
            break;
        case PARSE_STATE_QUOTED_VALUE:
            if(c == '"') {
                /* We are probably at the end of a name-value pair where the
                 * value is in quotes. However, let's make sure the end quote is
                 * followed by a separator or by the end of the command line
                 * before processing the pair. If it is followed by random junk
                 * instead, this is not a valid option. */
                value.length = current - value.start;
                state = PARSE_STATE_END_QUOTE;
            }
            break;
        case PARSE_STATE_END_QUOTE:
            if(is_separator(c)) {
                /* We are at a separator that follows a value in quotes. This
                 * makes the option valid, so let's process it. */
                process_name_value_pair(&name, &value);
                state = PARSE_STATE_START;
            }
            else {
                /* We found random junk after the quoted value. This is an
                 * invalid option. */
                cmdline_errors |= CMDLINE_ERROR_JUNK_AFTER_ENDQUOTE;
                state = PARSE_STATE_RECOVER;
            }
            break;
        case PARSE_STATE_DASH1:
            if(c == '-') {
                /* We might be at the start of an option that starts with two
                 * dashes (on the second dash) or we might be on the second
                 * dash of a double dash that ends the kernel options. The next
                 * character will tell. */
                state = PARSE_STATE_DASH2;
            }
            else {
                /* We are at the start of an option that starts with a single
                 * dash. name.start has already been set in PARSE_STATE_START. */
                state = PARSE_STATE_NAME;
            }
            break;
        case PARSE_STATE_DASH2:
            if(is_separator(c)) {
                /* We just found a double dash by itself. We are done. The
                 * options that follow aren't ours. */
                done = true;
            }
            else {
                /* We are at the start of an option that starts with two dashes,
                 * right after the second dash. name.start has already been set
                 * in PARSE_STATE_START. */
                state = PARSE_STATE_START;
            }
            break;
        case PARSE_STATE_RECOVER:
            /* We transition to this state when we encounter certain parsing
             * errors. We attempt to recover by discarding characters until the
             * next separator, and then continuing parsing there.
             *
             * TODO can we handle the special case of a missing separator
             * followed by an option with a quoted value? */
            if(is_separator(c)) {
                state = PARSE_STATE_START;
            }
            break;
        }

        /* Make sure we are not creating an infinite loop. Some states need to
         * handle the NUL character specifically to make sure the last option
         * on the command line is handled correctly. However, for any state, we
         * are now done. */
        if(c == '\0' || done) {
            break;
        }

        ++pos;
    }
}
