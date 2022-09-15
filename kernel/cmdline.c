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
#include <string.h>

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

#define CMDLINE_ERROR_UNCLOSED_QUOTES           (1<<9)

typedef enum {
    PARSE_STATE_START,
    PARSE_STATE_NAME,
    PARSE_STATE_EQUAL,
    PARSE_STATE_VALUE,
    PARSE_STATE_VALUE_START_QUOTE,
    PARSE_STATE_QUOTED_VALUE,
    PARSE_STATE_END_QUOTE,
    PARSE_STATE_DASH1,
    PARSE_STATE_DASH2,
    PARSE_STATE_OPTION_START_QUOTE,
    PARSE_STATE_QUOTED_OPTION,
    PARSE_STATE_ARG_START,
    PARSE_STATE_ARG,
    PARSE_STATE_ARG_EQUAL,
    PARSE_STATE_ARG_VALUE,
    PARSE_STATE_ARG_QUOTED_VALUE,
    PARSE_STATE_ARG_VALUE_END_QUOTE,
    PARSE_STATE_ARG_START_QUOTE,
    PARSE_STATE_QUOTED_ARG,
    PARSE_STATE_ARG_END_QUOTE,
} parse_state_t;

typedef struct {
    const char *start;
    size_t      length;
} token_t;

typedef struct {
    const char  *name;
    int          enum_value;
} enum_def_t;

typedef struct {
    const char      *cmdline;
    parse_state_t    state;
    token_t          option;
    token_t          value;
    int              position;
    int              errors;
    bool             done;
    bool             has_option;
    bool             has_argument;
} parse_context_t;

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

static void initialize_context(parse_context_t *context, const char *cmdline) {
    context->cmdline        = cmdline;
    context->state          = PARSE_STATE_START;
    context->position       = 0;
    context->errors         = 0;
    context->done           = false;

    context->option.start   = NULL;
    context->option.length  = 0;
    context->value.start    = NULL;
    context->value.length   = 0;
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

static void mutate_context(parse_context_t *context) {
    bool has_action = false;

    context->has_argument   = false;
    context->has_option = false;

    while(!(has_action || context->done)) {
        const char *current = &context->cmdline[context->position];
        char c              = *current;

        /* TODO check how the 32-bit setup code behaves when it copies the command line. */
        if(context->position >= CMDLINE_MAX_VALID_LENGTH) {
            /* Command line is too long. The limiting factor here is the stack
             * size of the user loader and of the initial process, since we intend
             * to copy the options from this command line to their command line
             * and environment.
             *
             * Let's still continue parsing though so we have better chances to
             * have the right VGA and serial port options when we report this
             * error. */
            context->errors |= CMDLINE_ERROR_TOO_LONG;

            if(context->position >= CMDLINE_MAX_PARSE_LENGTH) {
                /* The command line is *way* too long, probably because the
                 * terminating NUL character is missing. Let's give up. */
                context->done = true;
                break;
            }
        }

        switch(context->state) {
        case PARSE_STATE_START:
            if(c == '"') {
                /* This is the opening quote of a quoted argument. */
                context->state = PARSE_STATE_OPTION_START_QUOTE;
            }
            else if(c == '-') {
                /* We might be at the start of an option that starts with one or
                 * more dashes, or we might also be at the start of the double
                 * dash that marks the end of the options parsed by the kernel.
                 * We will only know for sure later. */
                context->option.start   = current;
                context->state          = PARSE_STATE_DASH1;
            }
            else if(! is_separator(c)) {
                /* We are at the start of an option, possibly a name-value pair,
                 * i.e. a name and value separated by an equal sign. */
                context->option.start   = current;
                context->state          = PARSE_STATE_NAME;
            }
            break;
        case PARSE_STATE_NAME:
            if(c == '=') {
                /* We just encountered an equal sign, so we are at the end of
                 * the name in what looks like an name-value pair. */
                context->option.length  = current - context->option.start;
                context->state          = PARSE_STATE_EQUAL;
            }
            else if(is_separator(c)) {
                /* We did not encounter an equal sign, so this is an argument. */
                context->state          = PARSE_STATE_START;
                context->option.length  = current - context->option.start;
                context->has_argument   = true;
                has_action              = true;
            }
            break;
        case PARSE_STATE_EQUAL:
            /* We are right after an option's equal sign that separate the name
             * of the option and it's value. */
            if(is_separator(c)) {
                /* The value is an empty string. */
                context->state          = PARSE_STATE_START;
                context->value.start    = current;
                context->value.length   = 0;
                context->has_option = true;
                has_action              = true;
            }
            else if(c == '"') {
                /* Looks like this is going to be a value in quotes. The value
                 * will end with a closing quote, not with the next separator
                 * (i.e. space) or the end of the command line. */
                context->state = PARSE_STATE_VALUE_START_QUOTE;
            }
            else {
                /* We are at the start of an unquoted value. */
                context->value.start    = current;
                context->state          = PARSE_STATE_VALUE;
            }
            break;
        case PARSE_STATE_VALUE:
            if(is_separator(c)) {
                /* We are at the end of a name-value pair. Time to process it. */
                context->state          = PARSE_STATE_START;
                context->value.length   = current - context->value.start;
                context->has_option = true;
                has_action              = true;
            }
            break;
        case PARSE_STATE_VALUE_START_QUOTE:
            /* We are at the start of a value in quotes. This state is needed so
             * the value excludes the opening quote. */
            if(c == '"') {
                /* empty value in quotes */
                context->state          = PARSE_STATE_END_QUOTE;
                context->value.start    = current;
                context->value.length   = 0;
                context->has_option = true;
                has_action              = true;

            }
            else if(c == '\0') {
                /* We reached the end of the command line but we are still
                 * expecting a closing quote character. Let's report this as an
                 * error.
                 *
                 * No need to do anything else than that because the NUL
                 * character leads to context->done being set for any state (see
                 * after this switch-case statement). */
                context->errors |= CMDLINE_ERROR_UNCLOSED_QUOTES;
            }
            else {
                context->value.start    = current;
                context->state          = PARSE_STATE_QUOTED_VALUE;
            }
            break;
        case PARSE_STATE_QUOTED_VALUE:
            if(c == '"') {
                /* We are at the end of a name-value pair where the value is in
                 * quotes. Let's process it. */
                context->state          = PARSE_STATE_END_QUOTE;
                context->value.length   = current - context->value.start;
                context->has_option = true;
                has_action              = true;
            }
            else if(c == '\0') {
                context->errors |= CMDLINE_ERROR_UNCLOSED_QUOTES;
            }
            break;
        case PARSE_STATE_END_QUOTE:
            if(is_separator(c)) {
                /* We are at a separator that follows a value in quotes or a
                 * quoted option. */
                context->state = PARSE_STATE_START;
            }
            else {
                /* We found random junk after the quoted option/value. Let's
                 * flag the error and then try to continue parsing by assuming
                 * it's just a missing separator. */
                context->errors |= CMDLINE_ERROR_JUNK_AFTER_ENDQUOTE;

                /* The following is the same logic as the PARSE_STATE_START
                 * state. */
                if(c == '"') {
                    context->state          = PARSE_STATE_OPTION_START_QUOTE;
                }
                else if(c == '-') {
                    context->option.start   = current;
                    context->state          = PARSE_STATE_DASH1;
                }
                else {
                    context->option.start   = current;
                    context->state          = PARSE_STATE_NAME;
                }
            }
            break;
        case PARSE_STATE_DASH1:
            if(c == '-') {
                /* We might be at the start of an option that starts with two
                 * dashes (on the second dash) or we might be on the second
                 * dash of a double dash that ends the kernel options. The next
                 * character will tell. */
                context->state = PARSE_STATE_DASH2;
            }
            else {
                /* We are at the start of an option that starts with a single
                 * dash. name.start has already been set in PARSE_STATE_START. */
                context->state = PARSE_STATE_NAME;
            }
            break;
        case PARSE_STATE_DASH2:
            if(is_separator(c)) {
                /* We just found a double dash by itself. We are done with
                 * kernel options. Everything that follows is arguments for
                 * user space. */
                context->state = PARSE_STATE_ARG_START;
            }
            else {
                /* We are at the start of an option that starts with two dashes,
                 * right after the second dash. name.start has already been set
                 * in PARSE_STATE_START. */
                context->state = PARSE_STATE_START;
            }
            break;
        case PARSE_STATE_OPTION_START_QUOTE:
            if(c == '"') {
                /* empty argument in quotes */
                context->state          = PARSE_STATE_END_QUOTE;
                context->option.start   = current;
                context->option.length  = 0;
                context->has_argument   = true;
                has_action              = true;
            }
            else if(c == '\0') {
                context->errors |= CMDLINE_ERROR_UNCLOSED_QUOTES;
            }
            else {
                context->state          = PARSE_STATE_QUOTED_OPTION;
                context->option.start   = current;
            }
            break;
        case PARSE_STATE_QUOTED_OPTION:
            if(c == '"') {
                context->state          = PARSE_STATE_END_QUOTE;
                context->option.length  = current - context->option.start;
                context->has_argument   = true;
                has_action              = true;
            }
            else if(c == '\0') {
                context->errors |= CMDLINE_ERROR_UNCLOSED_QUOTES;
            }
            break;
        case PARSE_STATE_ARG_START:
            if(c == '"') {
                /* This is the opening quote of a quoted argument. */
                context->state = PARSE_STATE_ARG_START_QUOTE;
            }
            else if(!is_separator(c)) {
                context->option.start   = current;
                context->state          = PARSE_STATE_ARG;
            }
            break;
        case PARSE_STATE_ARG:
            if(c == '=') {
                /* Event though this is an argument for user space, we still
                 * treat the equal sign specially since the value that follows
                 * might be in quotes. */
                context->state = PARSE_STATE_ARG_EQUAL;
            }
            else if(is_separator(c)) {
                /* We did not encounter an equal sign, so this is an argument. */
                context->state          = PARSE_STATE_ARG_START;
                context->option.length  = current - context->option.start;
                context->has_argument   = true;
                has_action              = true;
            }
            break;
        case PARSE_STATE_ARG_EQUAL:
            if(is_separator(c)) {
                /* The argument ends with an equal sign. */
                context->state          = PARSE_STATE_ARG_START;
                context->option.length  = current - context->option.start;
                context->has_argument   = true;
                has_action              = true;
            }
            else if(c == '"') {
                /* Looks like this is going to be a value in quotes. The value
                 * will end with a closing quote, not with the next separator
                 * (i.e. space) or the end of the command line. */
                context->state = PARSE_STATE_ARG_QUOTED_VALUE;
            }
            else {
                /* The argument just continues after the equal sign.*/
                context->state = PARSE_STATE_ARG_VALUE;
            }
            break;
        case PARSE_STATE_ARG_VALUE:
            if(is_separator(c)) {
                /* End of unquoted argument. */
                context->state          = PARSE_STATE_ARG_START;
                context->option.length  = current - context->option.start;
                context->has_argument   = true;
                has_action              = true;
            }
            break;
        case PARSE_STATE_ARG_QUOTED_VALUE:
            if(c == '"') {
                /* We are at the end of an argument with a quoted value after
                 * the equal sign. Unlike other cases of quoted options or
                 * arguments, here, we want to include the closing quote as part
                 * of the argument. */
                context->state  = PARSE_STATE_ARG_VALUE_END_QUOTE;
            }
            else if(c == '\0') {
                context->errors |= CMDLINE_ERROR_UNCLOSED_QUOTES;
            }
            break;
        case PARSE_STATE_ARG_VALUE_END_QUOTE:
            /* Process the argument, including the closing quote. */
            context->option.length  = current - context->option.start;
            context->has_argument   = true;
            has_action              = true;

            if(is_separator(c)) {
                /* We are at a separator that follows an argument with a quoted
                 * value. */
                context->state = PARSE_STATE_ARG_START;
            }
            else {
                /* We found random junk after the quoted argument. Let's flag
                 * the error and then stop. Since we are after the --, there are
                 * no more kernel options coming, so there is no use in
                 * continuing parsing further. */
                context->errors |= CMDLINE_ERROR_JUNK_AFTER_ENDQUOTE;
                context->done   = true;
            }
            break;
        case PARSE_STATE_ARG_START_QUOTE:
            if(c == '"') {
                /* empty argument in quotes */
                context->state          = PARSE_STATE_ARG_END_QUOTE;
                context->option.start   = current;
                context->option.length  = 0;
                context->has_argument   = true;
                has_action              = true;
            }
            else if(c == '\0') {
                context->errors |= CMDLINE_ERROR_UNCLOSED_QUOTES;
            }
            else {
                context->state          = PARSE_STATE_QUOTED_ARG;
                context->option.start   = current;
            }
            break;
        case PARSE_STATE_QUOTED_ARG:
            if(c == '"') {
                /* We are at the end of a quoted argument. */
                context->state          = PARSE_STATE_ARG_END_QUOTE;
                context->option.length  = current - context->option.start;
                context->has_argument   = true;
                has_action              = true;
            }
            else if(c == '\0') {
                context->errors |= CMDLINE_ERROR_UNCLOSED_QUOTES;
            }
            break;
        case PARSE_STATE_ARG_END_QUOTE:
            if(is_separator(c)) {
                /* We are at a separator that follows a quoted argument. */
                context->state = PARSE_STATE_ARG_START;
            }
            else {
                /* We found random junk after the quoted argument. Let's flag
                 * the error and then stop. Since we are after the --, there are
                 * no more kernel options coming, so there is no use in
                 * continuing parsing further. */
                context->errors |= CMDLINE_ERROR_JUNK_AFTER_ENDQUOTE;
                context->done   = true;
            }
            break;
        }

        /* Make sure we are not creating an infinite loop.
         *
         * Some states need to handle the NUL character specifically to make
         * sure the last option on the command line is handled correctly.
         * However, in any state, once we encounter the terminating NUL, we are
         * done. */
        if(c == '\0') {
            context->done = true;
        }

        ++context->position;
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
static void process_name_value_pair(parse_context_t *context) {
    int opt_name;
    int opt_value;

    if(! match_enum(&opt_name, opt_names, &context->option)) {
        /* Unknown option - ignore */
        return;
    }

    switch(opt_name) {
    case CMDLINE_OPT_NAME_PAE:
        if(match_enum(&opt_value, opt_pae_names, &context->value)) {
            cmdline_options.pae = opt_value;
        }
        else {
            context->errors |= CMDLINE_ERROR_INVALID_PAE;
        }
        break;
    case CMDLINE_OPT_NAME_SERIAL_ENABLE:
        if(match_enum(&opt_value, bool_names, &context->value)) {
            cmdline_options.serial_enable = opt_value;
        }
        else {
            context->errors |= CMDLINE_ERROR_INVALID_SERIAL_ENABLE;
        }
        break;
    case CMDLINE_OPT_NAME_SERIAL_BAUD_RATE:
        if(match_enum(&opt_value, serial_baud_rates, &context->value)) {
            cmdline_options.serial_baud_rate = opt_value;
        }
        else {
            context->errors |= CMDLINE_ERROR_INVALID_SERIAL_BAUD_RATE;
        }
        break;
    case CMDLINE_OPT_NAME_SERIAL_IOPORT:
        if(match_integer(&opt_value, &context->value)) {
            if(opt_value > SERIAL_MAX_IOPORT) {
                context->errors |= CMDLINE_ERROR_INVALID_SERIAL_IOPORT;
            }
            else {
                cmdline_options.serial_ioport = opt_value;
            }
        }
        else {
            context->errors |= CMDLINE_ERROR_INVALID_SERIAL_IOPORT;
        }
        break;
    case CMDLINE_OPT_NAME_SERIAL_DEV:
        if(match_enum(&opt_value, serial_ports, &context->value)) {
            cmdline_options.serial_ioport = opt_value;
        }
        else {
            context->errors |= CMDLINE_ERROR_INVALID_SERIAL_DEV;
        }
        break;
    case CMDLINE_OPT_NAME_VGA_ENABLE:
        if(match_enum(&opt_value, bool_names, &context->value)) {
            cmdline_options.vga_enable = opt_value;
        }
        else {
            context->errors |= CMDLINE_ERROR_INVALID_VGA_ENABLE;
        }
        break;
    }
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
 * Some options affect logging (e.g. VGA and/or serial port enabled, baud rate,
 * etc.). For this reason:
 * - This function will do its best to continue parsing options once parsing
 *   errors are encountered, to maximize chances we will get the logging options
 *   right.
 * - The actual reporting of parsing errors is done by a separate function, i.e.
 *   cmdline_report_parsing_errors(). This makes it possible to first parse the
 *   command line, then initialize logging with (hopefully) the right options,
 *   and then report parsing errors.
 *
 * @param cmdline command line string
 *
 * */
void cmdline_parse_options(const char *cmdline) {
    parse_context_t context;

    /* This function is the first thing that gets called during kernel
     * initialization. At this point, no validation has been done on the boot
     * information structure which contains the command line argument pointer. */
    if(cmdline == NULL) {
        /* cmdline_errors keeps track of errors for later reporting by the
         * cmdline_process_errors() function. */
        cmdline_errors |= CMDLINE_ERROR_IS_NULL;
        return;
    }

    initialize_context(&context, cmdline);

    do {
        mutate_context(&context);

        if(context.has_option) {
            process_name_value_pair(&context);
        }
    } while (!context.done);

    cmdline_errors = context.errors;
}

/**
 * Get the kernel command line options parsed with cmdline_parse_options().
 *
 * If called before cmdline_parse_options(), the returned options structure
 * contains the defaults.
 *
 * @return options parsed from kernel command line
 *
 * */
const cmdline_opts_t *cmdline_get_options(void) {
    return &cmdline_options;
}

/**
 * Report command line parsing errors
 *
 * Call this function after parsing the command line, i.e. after calling
 * cmdline_parse_options(), once the console (i.e. logging) has been initialized.
 * It logs a message for any parsing error that occurred and then panics if any
 * parsing error was fatal.
 *
 * This two-step process is needed because some command line arguments influence
 * how logging is performed (enable VGA and/or serial logging, baud rate, etc.).
 * For this reason, we want to first parse the command line, then initialize the
 * console (taking command line options into account), and then report parsing
 * errors.
 *
 * */
void cmdline_report_parsing_errors(void) {
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

    if(cmdline_errors & CMDLINE_ERROR_UNCLOSED_QUOTES) {
        printk("    Unclosed quotes at end of input. Is closing quote missing?\n");
    }

    if(cmdline_errors != 0) {
        panic("Invalid kernel command line");
    }
}

/* Write a character into buffer
 *
 * @param buffer address where writing starts
 * @param c character to write
 * @return pointer to first byte after written data
 *
 * */
static char *write_character(char *buffer, int c) {
    *buffer = c;
    return buffer + 1;
}

/* Write a token into buffer
 *
 * @param buffer address where writing starts
 * @param token token describing data to write
 * @return pointer to first byte after written data
 *
 * */
static char *write_token(char *buffer, const token_t *token) {
    memcpy(buffer, token->start, token->length);
    return buffer + token->length;
}

/* Write arguments parsed from the kernel command line into buffer
 *
 * This function assumes the command line is valid, i.e. that it has been
 * checked by earlier calls to cmdline_parse_options() and
 * cmdline_report_parsing_errors().
 *
 * @param buffer buffer where arguments are written
 * @param cmdline command line string
 * @return pointer to first byte after written data
 *
 * */
char *cmdline_write_arguments(char *buffer, const char *cmdline) {
    parse_context_t context;

    initialize_context(&context, cmdline);

    do {
        mutate_context(&context);

        if(context.has_argument) {
            buffer = write_token(buffer, &context.option);
            buffer = write_character(buffer, '\0');
        }
    } while (!context.done);

    return buffer;
}

/* Write environment variables parsed from the kernel command line into buffer
 *
 * This function assumes the command line is valid, i.e. that it has been
 * checked by earlier calls to cmdline_parse_options() and
 * cmdline_report_parsing_errors().
 *
 * @param buffer buffer where the environment variables are written
 * @param cmdline command line string
 * @return pointer to first byte after written data
 *
 * */
char *cmdline_write_environ(char *buffer, const char *cmdline) {
    parse_context_t context;

    initialize_context(&context, cmdline);

    do {
        mutate_context(&context);

        if(context.has_option) {
            buffer = write_token(buffer, &context.option);
            buffer = write_character(buffer, '=');
            buffer = write_token(buffer, &context.value);
            buffer = write_character(buffer, '\0');
        }
    } while (!context.done);

    return buffer;
}
