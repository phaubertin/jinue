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

typedef struct {
    const char  *format;
    char        *wrptr;
    size_t       len;
    size_t       maxlen;
    char        *saved_wrptr;
    size_t       saved_len;
} state_t;

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
    bool                plus;
    bool                minus;
    bool                zero;
    bool                space;
    bool                hash;
    int                 width;
    int                 prec;
    int                 bytes;  /* TODO delete */
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
    case LENGTH_MODIFIER_HH:
        return va_arg(*args, int);
    case LENGTH_MODIFIER_H:
        return va_arg(*args, int);
    case LENGTH_MODIFIER_L:
        return va_arg(*args, long int);
    case LENGTH_MODIFIER_LL:
        return va_arg(*args, long long int);
    case LENGTH_MODIFIER_J:
        return va_arg(*args, intmax_t);
    case LENGTH_MODIFIER_Z:
        return (intmax_t)va_arg(*args, size_t);
    case LENGTH_MODIFIER_NONE:
    default:
        return va_arg(*args, int);
    }
}

static uintmax_t get_unsigned_argument(state_t *state, const conv_spec_t *spec, va_list *args) {
    switch(spec->length_modifier) {
    case LENGTH_MODIFIER_HH:
        return va_arg(*args, int);
    case LENGTH_MODIFIER_H:
        /* TODO check this promotion */
        return va_arg(*args, int);
    case LENGTH_MODIFIER_L:
        return va_arg(*args, unsigned long int);
    case LENGTH_MODIFIER_LL:
        return va_arg(*args, unsigned long long int);
    case LENGTH_MODIFIER_J:
        return va_arg(*args, uintmax_t);
    case LENGTH_MODIFIER_Z:
        return va_arg(*args, size_t);
    case LENGTH_MODIFIER_NONE:
    default:
        return va_arg(*args, unsigned int);
    }
}

static void write_char(state_t *state, int c) {
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

static void write_unsigned(state_t *state, unsigned long long value, const conv_spec_t *spec) {
    if(value == 0) {
        write_char(state,  '0');
        return;
    }

    unsigned long long power;
    int digit;

    switch(length_modifier_size(spec)) {
    case 8:
        power   = 10000000000000000000ULL;
        digit   = 20;
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
        unsigned long long digit_value = value / power;

        if(nonzero || digit_value != 0 || digit <= spec->prec) {
            write_char(state, digit_value + '0');
            nonzero = true;
            value -= digit_value * power;
        }

        power /= 10;
        --digit;
    }
}

static void write_signed(state_t *state, long long value, const conv_spec_t *spec) {
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

static int parse_numeric(state_t *state) {
    int value = 0;

    while(isdigit(*state->format)) {
        value *= 10;
        value += *state->format - '0';
        ++(state->format);
    }

    return value;
}

static void parse_flags(conv_spec_t *spec, state_t *state) {
    spec->plus  = false;
    spec->minus = false;
    spec->zero  = false;
    spec->space = false;
    spec->hash  = false;

    while(process_flag(spec, *state->format)) {
        ++(state->format);
    }
}

static void parse_field_width(conv_spec_t *spec, state_t *state, va_list *args) {
    spec->width = -1;

    if(*state->format == '*') {
        ++(state->format);
        spec->width = va_arg(*args, int);
    }
    else if(isdigit(*state->format)) {
        spec->width = parse_numeric(state);
    }
}

static void parse_precision(conv_spec_t *spec, state_t *state, va_list *args) {
    spec->prec = -1;

    if(*state->format == '.') {
        ++(state->format);

        if(*state->format == '*') {
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
    switch(*state->format) {
    case 'h':
        spec->length_modifier = LENGTH_MODIFIER_H;
        ++(state->format);

        if(*state->format == 'h') {
            spec->length_modifier = LENGTH_MODIFIER_HH;
            ++(state->format);
        }
        break;
    case 'l':
        spec->length_modifier = LENGTH_MODIFIER_L;
        ++(state->format);

        if(*state->format == 'l') {
            spec->length_modifier = LENGTH_MODIFIER_LL;
            ++(state->format);
        }
        break;
    case 'j':
        spec->length_modifier = LENGTH_MODIFIER_J;
        ++(state->format);
        break;
    case 'z':
        spec->length_modifier = LENGTH_MODIFIER_Z;
        ++(state->format);
        break;
    default:
        spec->length_modifier = LENGTH_MODIFIER_NONE;
    }
}

static void parse_conversion_specifier(conv_spec_t *spec, state_t *state) {
    spec->conversion = *state->format;

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
        ++(state->format);
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

int vsnprintf(char *restrict s, size_t n, const char *restrict format, va_list args) {
    if(n == 0) {
        return 0;
    }

    state_t state;
    conv_spec_t spec;

    state.format    = format;
    state.wrptr     = s;
    state.len       = 0;
    state.maxlen    = n - 1;

    while(*state.format != '\0') {
        if(*state.format != '%') {
            write_char(&state, *state.format);
            ++(state.format);
            continue;
        }

        /* consume "%" */
        ++(state.format);

        if(*state.format == '%') {
            /* got "%%" */
            write_char(&state, '%');
            ++(state.format);
            continue;
        }

        parse_conversion_specification(&spec, &state, &args);
        process_conversion(&state, &spec, &args);
    }

    *state.wrptr = '\0';

    return state.len;
}
