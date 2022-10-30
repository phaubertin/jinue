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
} state_t;

typedef struct {
    char        *wrptr;
    size_t       len;
} saved_position_t;

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

static void save_position(saved_position_t *start, const state_t *state) {
    start->len      = state->len;
    start->wrptr    = state->wrptr;
}

static size_t get_length(const saved_position_t *start, const state_t *state) {
    return state->len - start->len;
}

static void start_dry_run(state_t *state) {
    state->wrptr = NULL;
}

static size_t go_back(state_t *state, const saved_position_t *start) {
    size_t length   = get_length(start, state);

    state->len      = start->len;
    state->wrptr    = start->wrptr;

    return length;
}

static int argument_size(const conv_spec_t *spec) {
    if(spec->conversion == 'p') {
        return sizeof(void *);
    }

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
    saved_position_t start;

    int prec            = spec->prec;
    const char *ptr     = str;
    save_position(&start, state);

    /* A precision less than zero (specifically, -1) means it wasn't specified,
     * and that, in turn, means we don't put a limit on the number of characters
     * we write. */
    while((prec < 0 || get_length(&start, state) < prec) && *ptr != '\0') {
        write_char(state, *ptr++);
    }
}

static void write_hexadecimal(state_t *state, uintmax_t value, const conv_spec_t *spec) {
    int prec = spec->prec;

    if(prec < 1) {
        /* A precision less than zero (specifically, -1) means no precision was
         * specified, in which case the default must be 1.
         *
         * As for the case prec == 0: there is no value that can be represented
         * with no digits, so specifying 0 or 1 as the precision is equivalent,
         * but the implementation below relies on the precision being at least
         * one. The specific case where this matters is when the value being
         * converted is zero, in which case we want to keep at least one zero
         * instead of discarding all zero-valued digits as leading zeroes. */
        prec = 1;
    }

    int digit = 2 * argument_size(spec);

    /* For pointers we always print all leading zeroes. (ISO/IEC 9899:TC2
     * §7.19.6.1 paragraph 8 lets us do whatever we want for this conversion.) */
    if(spec->conversion == 'p') {
       prec = digit;
    }

    if(spec->hash && (value != 0 || spec->conversion == 'p')) {
        /* "0x" prefix */
        write_char(state, '0');
        write_char(state, 'x');
    }

    for(int idx = 0; idx < prec - digit; ++idx) {
        write_char(state, '0');
    }

    bool nonzero = false;

    while(digit > 0) {
        int digit_bit_position = 4 * (digit - 1);
        int digit_value = (value >> digit_bit_position) & 0xf;

        if(nonzero || digit_value != 0 || digit <= prec) {
            if(digit_value < 10) {
                write_char(state, digit_value + '0');
            }
            else if(spec->conversion == 'X') {
                write_char(state, digit_value - 10 + 'A');
            }
            else {
                write_char(state, digit_value - 10 + 'a');
            }
            nonzero = true;
        }

        --digit;
    }
}

static void write_unsigned(state_t *state, uintmax_t value, const conv_spec_t *spec) {
    uintmax_t power;
    int digit;

    int prec = spec->prec;

    if(prec < 1) {
        /* A precision less than zero (specifically, -1) means no precision was
         * specified, in which case the default must be 1.
         *
         * As for the case prec == 0: there is no value that can be represented
         * with no digits, so specifying 0 or 1 as the precision is equivalent,
         * but the implementation below relies on the precision being at least
         * one. The specific case where this matters is when the value being
         * converted is zero, in which case we want to keep at least one zero
         * instead of discarding all zero-valued digits as leading zeroes. */
        prec = 1;
    }

    switch(argument_size(spec)) {
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

    for(int idx = 0; idx < prec - digit; ++idx) {
        write_char(state, '0');
    }

    bool nonzero = false;

    while(digit > 0) {
        uintmax_t digit_value = value / power;

        if(nonzero || digit_value != 0 || digit <= prec) {
            write_char(state, digit_value + '0');
            value -= digit_value * power;
            nonzero = true;
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

    bool is_flag = true;

    while(is_flag) {
        switch(current(state)) {
        case '-':
            spec->minus = true;
            consume(state);
            continue;
        case '+':
            spec->plus = true;
            consume(state);
            continue;
        case ' ':
            spec->space = true;
            consume(state);
            continue;
        case '#':
            spec->hash = true;
            consume(state);
            continue;
        case '0':
            spec->zero = true;
            consume(state);
            continue;
        default:
            is_flag = false;
        }
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

static void add_padding_right_justified(state_t *state, const conv_spec_t *spec, int length) {
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

static void add_padding_left_justified(state_t *state, const conv_spec_t *spec, int length) {
    if(spec->width < length) {
        return;
    }

    for(int idx = 0; idx < spec->width - length; ++idx) {
        write_char(state, ' ');
    }
}

static void process_conversion(state_t *state, const conv_spec_t *spec, va_list *args) {
    saved_position_t start;

    save_position(&start, state);

    switch(spec->conversion) {
    case 'i':
    case 'd':
    {
        intmax_t value = get_signed_argument(state, spec, args);

        if(is_right_justified(spec)) {
            start_dry_run(state);
            write_signed(state, value, spec);
            add_padding_right_justified(state, spec, go_back(state, &start));
        }

        write_signed(state, value, spec);

        if(is_left_justified(spec)) {
            add_padding_left_justified(state, spec, get_length(&start, state));
        }
    }
        break;
    case 'u':
    {
        uintmax_t value = get_unsigned_argument(state, spec, args);

        if(is_right_justified(spec)) {
            start_dry_run(state);
            write_unsigned(state, value, spec);
            add_padding_right_justified(state, spec, go_back(state, &start));
        }

        write_unsigned(state, value, spec);

        if(is_left_justified(spec)) {
            add_padding_left_justified(state, spec, get_length(&start, state));
        }
    }
        break;
    case 'x':
    case 'X':
    {
        uintmax_t value = get_unsigned_argument(state, spec, args);

        if(is_right_justified(spec)) {
            start_dry_run(state);
            write_hexadecimal(state, value, spec);
            add_padding_right_justified(state, spec, go_back(state, &start));
        }

        write_hexadecimal(state, value, spec);

        if(is_left_justified(spec)) {
            add_padding_left_justified(state, spec, get_length(&start, state));
        }
    }
        break;
    case 'c':
    {
        const unsigned char value = (unsigned char)va_arg(*args, int);

        if(is_right_justified(spec)) {
            add_padding_right_justified(state, spec, 1);
        }

        write_char(state, value);

        if(is_left_justified(spec)) {
            add_padding_left_justified(state, spec, 1);
        }
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
            add_padding_right_justified(state, spec, go_back(state, &start));
        }

        write_string(state, value, spec);

        if(is_left_justified(spec)) {
            add_padding_left_justified(state, spec, get_length(&start, state));
        }
    }
        break;
    case 'p':
    {
        void *value = va_arg(*args, void *);

        if(is_right_justified(spec)) {
            start_dry_run(state);
            write_hexadecimal(state, (uintptr_t)value, spec);
            add_padding_right_justified(state, spec, go_back(state, &start));
        }

        write_hexadecimal(state, (uintptr_t)value, spec);

        if(is_left_justified(spec)) {
            add_padding_left_justified(state, spec, get_length(&start, state));
        }
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
