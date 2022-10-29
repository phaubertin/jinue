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

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>


typedef enum {
    LENGTH_MODIFIER_NONE,
    LENGTH_MODIFIER_HH,
    LENGTH_MODIFIER_H,
    LENGTH_MODIFIER_L,
    LENGTH_MODIFIER_LL,
    LENGTH_MODIFIER_J,
    LENGTH_MODIFIER_Z
} length_modifier_t;

typedef struct {
    const char  *format;
    char        *wrptr;
    size_t       len;
    size_t       maxlen;
    char        *saved_wrptr;
    size_t       saved_len;
} state_t;

typedef struct {
    bool                plus;
    bool                minus;
    bool                zero;
    bool                space;
    bool                hash;
    int                 width;
    int                 prec;
    length_modifier_t   length_modifier;
    int                 conversion;
} conv_spec_t;

static void start_dry_run(state_t *state) {
    state->saved_len    = state->len;
    state->saved_wrptr  = state->wrptr;
    state->wrptr        = NULL;
}

static size_t end_dry_run_and_get_length(state_t *state) {
    size_t length   = state->len - state->saved_len;

    state->len      = state->saved_len;
    state->wrptr    = state->saved_wrptr;

    return length;
}

static int get_start(state_t *state) {
    return state->len;
}

static size_t get_length(state_t *state, int start) {
    return state->len - start;
}

static int length_modifier_size(const conv_spec_t *spec) {
    switch(spec->length_modifier) {
    case LENGTH_MODIFIER_HH:
        return sizeof(char);
    case LENGTH_MODIFIER_H:
        return sizeof(short int);
    case LENGTH_MODIFIER_L:
        return sizeof(long int);
    case LENGTH_MODIFIER_LL:
        return sizeof(long long int);
    case LENGTH_MODIFIER_J:
        return sizeof(intmax_t);
    case LENGTH_MODIFIER_Z:
        return sizeof(size_t);
    case LENGTH_MODIFIER_NONE:
    default:
        return sizeof(int);
    }
}

static intmax_t get_signed_argument(state_t *state, const conv_spec_t *spec, va_list *args) {
    switch(spec->length_modifier) {
    case LENGTH_MODIFIER_L:
        return va_arg(*args, long int);
    case LENGTH_MODIFIER_LL:
        return va_arg(*args, long long int);
    case LENGTH_MODIFIER_J:
        return va_arg(*args, intmax_t);
    case LENGTH_MODIFIER_Z:
        return (intmax_t)va_arg(*args, size_t);
    case LENGTH_MODIFIER_NONE:
    case LENGTH_MODIFIER_HH:
    case LENGTH_MODIFIER_H:
    default:
        return va_arg(*args, int);
    }
}

static uintmax_t get_unsigned_argument(state_t *state, const conv_spec_t *spec, va_list *args) {
    switch(spec->length_modifier) {
    case LENGTH_MODIFIER_L:
        return va_arg(*args, unsigned long int);
    case LENGTH_MODIFIER_LL:
        return va_arg(*args, unsigned long long int);
    case LENGTH_MODIFIER_J:
        return va_arg(*args, uintmax_t);
    case LENGTH_MODIFIER_Z:
        return va_arg(*args, size_t);
    case LENGTH_MODIFIER_NONE:
    case LENGTH_MODIFIER_HH:
    case LENGTH_MODIFIER_H:
    default:
        return va_arg(*args, unsigned int);
    }
}

static void terminate_string(state_t *state) {
    /* All writes to the output buffer except this one go through write_char().
     * That function ensures there is space to write the terminating null
     * character. */
    if(state->wrptr != NULL) {
        *state->wrptr = '\0';
    }
}

static void write_char(state_t *state, int c) {
    /* All writes to the output buffer except writing the terminating null
     * character go through this function (see terminate_string()). Here, we
     * must reserve the space necessary to store that terminating character.*/
    if(state->len < state->maxlen && state->wrptr != NULL) {
        *(state->wrptr)++ = c;
    }

    ++state->len;
}

static void write_string(state_t *state, const char *str, const conv_spec_t *spec) {
    int prec        = spec->prec;
    const char *ptr = str;
    size_t start    = get_start(state);

    while((prec < 0 || get_length(state, start) < prec) && *ptr != '\0') {
        write_char(state, *ptr++);
    }
}

static void write_unsigned(state_t *state, uintmax_t value, const conv_spec_t *spec) {
    if(value == 0) {
        write_char(state,  '0');
        return;
    }

    uintmax_t power;
    int digit;

    switch(length_modifier_size(spec)) {
    case 8:
        power    = UINTMAX_C(10000000000000000000);
        digit    = 20;
        break;
    case 4:
        power    = 1000000000;
        value   %= 4294967296;
        digit    = 10;
        break;
    case 2:
        power    = 10000;
        value   %= 65536;
        digit    = 5;
        break;
    default:
        power    = 100;
        value   %= 256;
        digit    = 3;
    }

    for(int idx = 0; idx < spec->prec - digit; ++idx) {
        write_char(state, '0');
    }

    bool nonzero = false;

    while(power > 0) {
        uintmax_t digit_value = value / power;

        if(nonzero || digit_value != 0 || digit <= spec->prec) {
            write_char(state, digit_value + '0');
            nonzero = true;
            value -= digit_value * power;
        }

        power /= 10;
        --digit;
    }
}

static void write_signed(state_t *state, intmax_t value, const conv_spec_t *spec) {
    if(value < 0) {
        write_char(state, '-');
        write_unsigned(state, -value, spec);
    }
    else {
        if(spec->plus) {
            write_char(state, '+');
        }
        else if(spec->space) {
            write_char(state, ' ');
        }
        write_unsigned(state, value, spec);
    }
}

static bool process_flag(conv_spec_t *spec, int c) {
    switch(c) {
    case '-':
        spec->minus = true;
        return true;
    case '+':
        spec->plus = true;
        return true;
    case ' ':
        spec->space = true;
        return true;
    case '#':
        spec->hash = true;
        return true;
    case '0':
        spec->zero = true;
        return true;
    default:
        return false;
    }
}

static int current(const state_t *state) {
    return *state->format;
}

static void consume(state_t *state) {
    ++(state->format);
}

static int parse_numeric(state_t *state) {
    int value = 0;

    while(isdigit(current(state))) {
        value *= 10;
        value += current(state) - '0';
        consume(state);
    }

    return value;
}

static void parse_flags(conv_spec_t *spec, state_t *state) {
    spec->plus  = false;
    spec->minus = false;
    spec->zero  = false;
    spec->space = false;
    spec->hash  = false;

    while(process_flag(spec, current(state))) {
        consume(state);
    }
}

static void parse_field_width(conv_spec_t *spec, state_t *state, va_list *args) {
    spec->width = -1;

    if(current(state) == '*') {
        consume(state);
        spec->width = va_arg(*args, int);
    }
    else if(isdigit(current(state))) {
        spec->width = parse_numeric(state);
    }
}

static void parse_precision(conv_spec_t *spec, state_t *state, va_list *args) {
    spec->prec = -1;

    if(current(state) == '.') {
        consume(state);

        if(current(state) == '*') {
            spec->prec = va_arg(*args, int);

            if(spec->prec < 0) {
                spec->prec  = -spec->prec;
                spec->minus = true;
            }
        }
        else {
            spec->prec = parse_numeric(state);
        }
    }
}

static void parse_length_modifier(conv_spec_t *spec, state_t *state) {
    switch(current(state)) {
    case 'h':
        spec->length_modifier = LENGTH_MODIFIER_H;
        consume(state);

        if(current(state) == 'h') {
            spec->length_modifier = LENGTH_MODIFIER_HH;
            consume(state);
        }
        break;
    case 'l':
        spec->length_modifier = LENGTH_MODIFIER_L;
        consume(state);

        if(current(state) == 'l') {
            spec->length_modifier = LENGTH_MODIFIER_LL;
            consume(state);
        }
        break;
    case 'j':
        spec->length_modifier = LENGTH_MODIFIER_J;
        consume(state);
        break;
    case 'z':
        spec->length_modifier = LENGTH_MODIFIER_Z;
        consume(state);
        break;
    default:
        spec->length_modifier = LENGTH_MODIFIER_NONE;
    }
}

static void parse_conversion_specifier(conv_spec_t *spec, state_t *state) {
    spec->conversion = current(state);

    /* Only consume conversion specifier if it is recognized. */

    switch(spec->conversion) {
    case 'd':
    case 'i':
    case 'u':
    case 'x':
    case 'X':
    case 'c':
    case 's':
    case 'p':
        consume(state);
    }
}

static void parse_conversion_specification(conv_spec_t *spec, state_t *state, va_list *args) {
    parse_flags(spec, state);
    parse_field_width(spec, state, args);
    parse_precision(spec, state, args);
    parse_length_modifier(spec, state);
    parse_conversion_specifier(spec, state);
}

static bool is_right_justified(const conv_spec_t *spec) {
    return spec->width > 0 && !spec->minus;
}

static void right_justify(state_t *state, const conv_spec_t *spec, int length) {
    if(!is_right_justified(spec)) {
        return;
    }

    if(spec->width < length) {
        return;
    }

    int padchar = ' ';

    if(spec->prec < 0 && spec->zero) {
        padchar = '0';
    }

    for(int idx = 0; idx < spec->width - length; ++idx) {
        write_char(state, padchar);
    }
}

static bool is_left_justified(const conv_spec_t *spec) {
    return spec->width > 0 && spec->minus;
}

static void left_justify(state_t *state, const conv_spec_t *spec, int length) {
    if(!is_left_justified(spec)) {
        return;
    }

    if(spec->width < length) {
        return;
    }

    for(int idx = 0; idx < spec->width - length; ++idx) {
        write_char(state, ' ');
    }
}

static void process_conversion(state_t *state, const conv_spec_t *spec, va_list *args) {
    switch(spec->conversion) {
    case 'i':
    case 'd':
    {
        intmax_t value = get_signed_argument(state, spec, args);

        if(is_right_justified(spec)) {
            start_dry_run(state);
            write_signed(state, value, spec);
            right_justify(state, spec, end_dry_run_and_get_length(state));
        }

        int start = get_start(state);
        write_signed(state, value, spec);
        left_justify(state, spec, get_length(state, start));
    }
        break;
    case 'u':
    {
        uintmax_t value = get_unsigned_argument(state, spec, args);

        if(is_right_justified(spec)) {
            start_dry_run(state);
            write_unsigned(state, value, spec);
            right_justify(state, spec, end_dry_run_and_get_length(state));
        }

        int start = get_start(state);
        write_unsigned(state, value, spec);
        left_justify(state, spec, get_length(state, start));
    }
        break;
    case 's':
    {
        const char *value = va_arg(*args, char *);

        if(value == NULL) {
            value = "(null)";
        }

        if(is_right_justified(spec)) {
            start_dry_run(state);
            write_string(state, value, spec);
            right_justify(state, spec, end_dry_run_and_get_length(state));
        }

        int start = get_start(state);
        write_string(state, value, spec);
        left_justify(state, spec, get_length(state, start));
    }
        break;
    default:
        break;
    }
}

static void initialize_state(state_t *state, char *s, size_t n, const char *format) {
    state->format   = format;
    state->wrptr    = s;
    state->len      = 0;
    state->maxlen   = (n == 0) ? 0 : n - 1;
}

int vsnprintf(char *restrict s, size_t n, const char *restrict format, va_list args) {
    state_t state;
    conv_spec_t spec;

    initialize_state(&state, s, n, format);

    while(current(&state) != '\0') {
        if(current(&state) != '%') {
            write_char(&state, current(&state));
            consume(&state);
            continue;
        }

        /* consume "%" */
        consume(&state);

        if(current(&state) == '%') {
            /* got "%%" */
            write_char(&state, '%');
            consume(&state);
            continue;
        }

        parse_conversion_specification(&spec, &state, &args);
        process_conversion(&state, &spec, &args);
    }

    terminate_string(&state);

    return state.len;
}
