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
#include <kernel/domain/services/panic.h>
#include <kernel/machine/cmdline.h>
#include <ctype.h>
#include <string.h>

#define CMDLINE_ERROR_TOO_LONG                  (1<<0)

#define CMDLINE_ERROR_IS_NULL                   (1<<1)

#define CMDLINE_ERROR_JUNK_AFTER_ENDQUOTE       (1<<2)

#define CMDLINE_ERROR_UNCLOSED_QUOTES           (1<<3)

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
    PARSE_STATE_ARGUMENT_START_QUOTE,
    PARSE_STATE_QUOTED_ARGUMENT,
    PARSE_STATE_AFTER2DASH_START,
    PARSE_STATE_AFTER2DASH_ARGUMENT,
    PARSE_STATE_AFTER2DASH_EQUAL,
    PARSE_STATE_AFTER2DASH_VALUE,
    PARSE_STATE_AFTER2DASH_QUOTED_VALUE,
    PARSE_STATE_AFTER2DASH_VALUE_END_QUOTE,
    PARSE_STATE_AFTER2DASH_START_QUOTE,
    PARSE_STATE_AFTER2DASH_QUOTED_ARGUMENT,
    PARSE_STATE_AFTER2DASH_END_QUOTE,
} parse_state_t;

typedef struct {
    const char      *cmdline;
    parse_state_t    state;
    cmdline_token_t  option;
    cmdline_token_t  value;
    int              position;
    bool             done;
    bool             has_option;
    bool             has_argument;
} parse_context_t;

static const cmdline_enum_def_t kernel_option_names[] = {
    {NULL, 0}
};

static const cmdline_enum_def_t bool_names[] = {
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

static int cmdline_errors;

static void initialize_context(parse_context_t *context, const char *cmdline) {
    context->cmdline        = cmdline;
    context->state          = PARSE_STATE_START;
    context->position       = 0;
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

    context->has_argument = false;
    context->has_option = false;

    while(!(has_action || context->done)) {
        const char *current = &context->cmdline[context->position];
        char c              = *current;

        if(context->position >= CMDLINE_MAX_VALID_LENGTH) {
            /* Command line is too long. The limiting factor here is space on
             * the user stack for command line arguments, environment variables
             * and associated indexing string arrays.
             *
             * Let's still continue parsing though. We will report the command
             * line as being too long and panic, but let's improve our chances
             * to have the right logging options when we do. */
            cmdline_errors |= CMDLINE_ERROR_TOO_LONG;

            if(context->position >= CMDLINE_MAX_PARSE_LENGTH) {
                /* The command line is *way* too long, probably because the
                 * terminating NUL character is missing. Let's give up.
                 *
                 * The setup code is supposed to do the right thing and crop
                 * and NUL terminate the string if it gets to this length.
                 * However, this is called very early during the initialization
                 * process, before the boot information structure from which the
                 * command line pointer is read has been validated, so let's not
                 * take any chances. */
                context->done = true;
                break;
            }
        }

        switch(context->state) {
        case PARSE_STATE_START:
            if(c == '"') {
                /* This is the opening quote of a quoted argument. */
                context->state = PARSE_STATE_ARGUMENT_START_QUOTE;
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
                cmdline_errors |= CMDLINE_ERROR_UNCLOSED_QUOTES;
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
                cmdline_errors |= CMDLINE_ERROR_UNCLOSED_QUOTES;
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
                cmdline_errors |= CMDLINE_ERROR_JUNK_AFTER_ENDQUOTE;

                /* The following is the same logic as the PARSE_STATE_START
                 * state. */
                if(c == '"') {
                    context->state          = PARSE_STATE_ARGUMENT_START_QUOTE;
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
                context->state = PARSE_STATE_AFTER2DASH_START;
            }
            else {
                /* We are at the start of an option that starts with two dashes,
                 * right after the second dash. name.start has already been set
                 * in PARSE_STATE_START. */
                context->state = PARSE_STATE_START;
            }
            break;
        case PARSE_STATE_ARGUMENT_START_QUOTE:
            if(c == '"') {
                /* empty argument in quotes */
                context->state          = PARSE_STATE_END_QUOTE;
                context->option.start   = current;
                context->option.length  = 0;
                context->has_argument   = true;
                has_action              = true;
            }
            else if(c == '\0') {
                cmdline_errors |= CMDLINE_ERROR_UNCLOSED_QUOTES;
            }
            else {
                context->state          = PARSE_STATE_QUOTED_ARGUMENT;
                context->option.start   = current;
            }
            break;
        case PARSE_STATE_QUOTED_ARGUMENT:
            if(c == '"') {
                context->state          = PARSE_STATE_END_QUOTE;
                context->option.length  = current - context->option.start;
                context->has_argument   = true;
                has_action              = true;
            }
            else if(c == '\0') {
                cmdline_errors |= CMDLINE_ERROR_UNCLOSED_QUOTES;
            }
            break;
        case PARSE_STATE_AFTER2DASH_START:
            if(c == '"') {
                /* This is the opening quote of a quoted argument. */
                context->state = PARSE_STATE_AFTER2DASH_START_QUOTE;
            }
            else if(!is_separator(c)) {
                context->option.start   = current;
                context->state          = PARSE_STATE_AFTER2DASH_ARGUMENT;
            }
            break;
        case PARSE_STATE_AFTER2DASH_ARGUMENT:
            if(c == '=') {
                /* Event though this is an argument for user space, we still
                 * treat the equal sign specially since the value that follows
                 * might be in quotes. */
                context->state = PARSE_STATE_AFTER2DASH_EQUAL;
            }
            else if(is_separator(c)) {
                /* We did not encounter an equal sign, so this is an argument. */
                context->state          = PARSE_STATE_AFTER2DASH_START;
                context->option.length  = current - context->option.start;
                context->has_argument   = true;
                has_action              = true;
            }
            break;
        case PARSE_STATE_AFTER2DASH_EQUAL:
            if(is_separator(c)) {
                /* The argument ends with an equal sign. */
                context->state          = PARSE_STATE_AFTER2DASH_START;
                context->option.length  = current - context->option.start;
                context->has_argument   = true;
                has_action              = true;
            }
            else if(c == '"') {
                /* Looks like this is going to be a value in quotes. The value
                 * will end with a closing quote, not with the next separator
                 * (i.e. space) or the end of the command line. */
                context->state = PARSE_STATE_AFTER2DASH_QUOTED_VALUE;
            }
            else {
                /* The argument just continues after the equal sign.*/
                context->state = PARSE_STATE_AFTER2DASH_VALUE;
            }
            break;
        case PARSE_STATE_AFTER2DASH_VALUE:
            if(is_separator(c)) {
                /* End of unquoted argument. */
                context->state          = PARSE_STATE_AFTER2DASH_START;
                context->option.length  = current - context->option.start;
                context->has_argument   = true;
                has_action              = true;
            }
            break;
        case PARSE_STATE_AFTER2DASH_QUOTED_VALUE:
            if(c == '"') {
                /* We are at the end of an argument with a quoted value after
                 * the equal sign. Unlike other cases of quoted options or
                 * arguments, here, we want to include the closing quote as part
                 * of the argument. */
                context->state  = PARSE_STATE_AFTER2DASH_VALUE_END_QUOTE;
            }
            else if(c == '\0') {
                cmdline_errors |= CMDLINE_ERROR_UNCLOSED_QUOTES;
            }
            break;
        case PARSE_STATE_AFTER2DASH_VALUE_END_QUOTE:
            /* Process the argument, including the closing quote. */
            context->option.length  = current - context->option.start;
            context->has_argument   = true;
            has_action              = true;

            if(is_separator(c)) {
                /* We are at a separator that follows an argument with a quoted
                 * value. */
                context->state = PARSE_STATE_AFTER2DASH_START;
            }
            else {
                /* We found random junk after the quoted argument. Let's flag
                 * the error and then stop. Since we are after the --, there are
                 * no more kernel options coming, so there is no use in
                 * continuing parsing further. */
                cmdline_errors |= CMDLINE_ERROR_JUNK_AFTER_ENDQUOTE;
                context->done   = true;
            }
            break;
        case PARSE_STATE_AFTER2DASH_START_QUOTE:
            if(c == '"') {
                /* empty argument in quotes */
                context->state          = PARSE_STATE_AFTER2DASH_END_QUOTE;
                context->option.start   = current;
                context->option.length  = 0;
                context->has_argument   = true;
                has_action              = true;
            }
            else if(c == '\0') {
                cmdline_errors |= CMDLINE_ERROR_UNCLOSED_QUOTES;
            }
            else {
                context->state          = PARSE_STATE_AFTER2DASH_QUOTED_ARGUMENT;
                context->option.start   = current;
            }
            break;
        case PARSE_STATE_AFTER2DASH_QUOTED_ARGUMENT:
            if(c == '"') {
                /* We are at the end of a quoted argument. */
                context->state          = PARSE_STATE_AFTER2DASH_END_QUOTE;
                context->option.length  = current - context->option.start;
                context->has_argument   = true;
                has_action              = true;
            }
            else if(c == '\0') {
                cmdline_errors |= CMDLINE_ERROR_UNCLOSED_QUOTES;
            }
            break;
        case PARSE_STATE_AFTER2DASH_END_QUOTE:
            if(is_separator(c)) {
                /* We are at a separator that follows a quoted argument. */
                context->state = PARSE_STATE_AFTER2DASH_START;
            }
            else {
                /* We found random junk after the quoted argument. Let's flag
                 * the error and then stop. Since we are after the --, there are
                 * no more kernel options coming, so there is no use in
                 * continuing parsing further. */
                cmdline_errors |= CMDLINE_ERROR_JUNK_AFTER_ENDQUOTE;
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
 * Retrieve enum definition entry by name represented by a token
 *
 * @param def enum definition
 * @param token name to search in enum definition
 * @return enum definition entry if found, NULL otherwise
 *
 * */
static const cmdline_enum_def_t *get_enum_entry_by_token(const cmdline_enum_def_t *def, const cmdline_token_t *token) {
    for(int def_index = 0; def[def_index].name != NULL; ++def_index) {
        int token_index;

        const cmdline_enum_def_t *def_entry = &def[def_index];

        /* Optimism, captain! */
        bool match = true;

        for(token_index = 0; token_index < token->length; ++token_index) {
            int name_c  = def_entry->name[token_index];
            int token_c = token->start[token_index];

            /* Underscore (_) and dash (-) are treated as equivalent. */
            if(name_c == '-') {
                name_c = '_';
            }

            if(token_c == '-') {
                token_c = '_';
            }

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
            if(def_entry->name[token_index] == '\0') {
                return def_entry;
            }
        }
    }

    /* No match */
    return NULL;
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
bool cmdline_match_enum(
        int                         *value,
        const cmdline_enum_def_t    *def,
        const cmdline_token_t       *token) {
    const cmdline_enum_def_t *entry = get_enum_entry_by_token(def, token);

    if(entry == NULL) {
        return false;
    }

    *value = entry->enum_value;
    return true;
}

/**
 * Attempt to match a boolean value
 *
 * @param value set to boolean value only if there is a match
 * @param token potential name representing a boolean value
 * @return true if a boolean value was successfully matched, false otherwise
 *
 * */
bool cmdline_match_boolean(bool *value, const cmdline_token_t *token) {
    int ivalue;

    bool matched = cmdline_match_enum(&ivalue, bool_names, token);

    if(matched) {
        *value = !!ivalue;
    }

    return matched;
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
bool cmdline_match_integer(int *ivalue, const cmdline_token_t *value) {
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
static void process_name_value_pair(config_t *config, parse_context_t *context) {
    int opt_name;

    if(! cmdline_match_enum(&opt_name, kernel_option_names, &context->option)) {
        machine_cmdline_process_option(
            &config->machine,
            &context->option,
            &context->value
        );
        return;
    }

    /* TODO process machine-independent options here (there aren't any yet) */
}

/**
 * Parse the kernel command line options into the kernel configuration
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
 *   cmdline_report_errors(). This makes it possible to first parse the
 *   command line, then initialize logging with (hopefully) the right options,
 *   and then report parsing errors.
 *
 * @param config kernel configuration
 * @param cmdline command line string
 *
 * */
void cmdline_parse_options(config_t *config, const char *cmdline) {
    cmdline_errors = 0;

    machine_cmdline_start_parsing(&config->machine);

    if(cmdline == NULL) {
        /* cmdline_errors keeps track of errors for later reporting by the
         * cmdline_process_errors() function. */
        cmdline_errors |= CMDLINE_ERROR_IS_NULL;
        return;
    }

    parse_context_t context;
    initialize_context(&context, cmdline);

    do {
        mutate_context(&context);

        if(context.has_option) {
            process_name_value_pair(config, &context);
        }
    } while (!context.done);
}

/**
 * Report command line parsing errors
 *
 * Call this function after parsing the command line, i.e. after calling
 * cmdline_parse_options(), once logging has been initialized. It logs a message
 * for any parsing error that occurred and then panics if any parsing error was
 * fatal.
 *
 * This two-step process is needed because some command line arguments influence
 * how logging is performed (enable VGA and/or serial logging, baud rate, etc.).
 * For this reason, we want to first parse the command line, then initialize
 * logging (taking command line options into account), and then report parsing
 * errors.
 *
 * */
void cmdline_report_errors(void) {
    if(cmdline_errors == 0 && !machine_cmdline_has_errors()) {
        return;
    }

    warning("There are issues with the kernel command line:");

    if(cmdline_errors & CMDLINE_ERROR_TOO_LONG) {
        warning("  Kernel command line is too long");
    }

    if(cmdline_errors & CMDLINE_ERROR_IS_NULL) {
        warning("  No kernel command line/command line is NULL");
    }

    machine_cmdline_report_errors();

    if(cmdline_errors & CMDLINE_ERROR_JUNK_AFTER_ENDQUOTE) {
        warning("  Invalid character after closing quote, separator (e.g. space) expected");
    }

    if(cmdline_errors & CMDLINE_ERROR_UNCLOSED_QUOTES) {
        warning("  Unclosed quotes at end of input. Is closing quote missing?");
    }

    panic("Invalid kernel command line");
}

/**
 * Write a character into buffer
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

/**
 * Write a token into buffer
 *
 * @param buffer address where writing starts
 * @param token token describing data to write
 * @return pointer to first byte after written data
 *
 * */
static char *write_token(char *buffer, const cmdline_token_t *token) {
    memcpy(buffer, token->start, token->length);
    return buffer + token->length;
}

/**
 * Write arguments parsed from the kernel command line into buffer
 *
 * This function assumes the command line is valid, i.e. that it has been
 * checked by earlier calls to cmdline_parse_options() and
 * cmdline_report_errors().
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

/**
 * Filter user space environment variables
 *
 * Kernel options are filtered out from user space environment variables.
 *
 * @param name name of the option
 * @return true if variable belongs in the user space environment, false otherwise
 *
 * */
static bool filter_userspace_environ(const cmdline_token_t *name) {
    /* Filter out kernel options from user space environment variables. */
    return get_enum_entry_by_token(kernel_option_names, name) == NULL;
}

/**
 * Write environment variables parsed from the kernel command line into buffer
 *
 * This function assumes the command line is valid, i.e. that it has been
 * checked by earlier calls to cmdline_parse_options() and
 * cmdline_report_errors().
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

        if(context.has_option && filter_userspace_environ(&context.option)) {
            buffer = write_token(buffer, &context.option);
            buffer = write_character(buffer, '=');
            buffer = write_token(buffer, &context.value);
            buffer = write_character(buffer, '\0');
        }
    } while (!context.done);

    return buffer;
}

/**
 * Count arguments parsed from the kernel command line
 *
 * This function assumes the command line is valid, i.e. that it has been
 * checked by earlier calls to cmdline_parse_options() and
 * cmdline_report_errors().
 *
 * @param cmdline command line string
 * @return number of arguments
 *
 * */
size_t cmdline_count_arguments(const char *cmdline) {
    parse_context_t context;

    size_t count = 0;
    initialize_context(&context, cmdline);

    do {
        mutate_context(&context);

        if(context.has_argument) {
            ++count;
        }
    } while (!context.done);

    return count;
}

/**
 * Count environment variables parsed from the kernel command line
 *
 * This function assumes the command line is valid, i.e. that it has been
 * checked by earlier calls to cmdline_parse_options() and
 * cmdline_report_errors().
 *
 * @param cmdline command line string
 * @return number of environment variables
 *
 * */
size_t cmdline_count_environ(const char *cmdline) {
    parse_context_t context;

    size_t count = 0;
    initialize_context(&context, cmdline);

    do {
        mutate_context(&context);

        if(context.has_option && filter_userspace_environ(&context.option)) {
            ++count;
        }
    } while (!context.done);

    return count;
}
