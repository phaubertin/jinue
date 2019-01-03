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

#include <jinue/console.h>
#include <printk.h>
#include <stdarg.h>
#include <stddef.h>


/** @cond PRINTK */
void printk(const char *format, ...) {
    va_list ap;
    const char *idx, *anchor;
    ptrdiff_t size;
    
    va_start(ap, format);
    
    idx = format;
    
    while(1) {
        anchor = idx;
        
        while( *idx != 0 && *idx != '%' ) {
            ++idx;
        }
        
        size = idx - anchor;
        
        if(size > 0) {
            console_printn(anchor, size);
        }
        
        if(*idx == 0 || *(idx+1) == 0) {
            break;
        }
        
        ++idx;        
        
        switch( *idx ) {
        case '%':
            console_putc('%');
            break;
        
        case 'c':
            console_putc( (char)va_arg(ap, int) );
            break;
        
        case 'q':
            print_hex_q( va_arg(ap, unsigned long long) );
            break;
        
        case 's':
            console_print( va_arg(ap, const char *) );
            break;
        
        case 'u':
            print_unsigned_int( va_arg(ap, unsigned int) );
            break;
        
        case 'b':
            print_hex_b( (unsigned char)va_arg(ap, unsigned long) );
            break;
        
        case 'w':
            print_hex_w( (unsigned short)va_arg(ap, unsigned long) );
            break;
        
        case 'x':
            print_hex_l( va_arg(ap, unsigned long) );
            break;
        
        default:
            va_end(ap);
            return;
        }
        
        ++idx;
    }    
    
    va_end(ap);        
}
/** @endcond PRINTK */

void print_unsigned_int(unsigned int n) {
    unsigned int flag = 0;
    unsigned int pwr;
    unsigned int digit;
    char c;
    
    if(n == 0) {
        console_putc('0');
        return;
    }
    
    for(pwr = 1000 * 1000 * 1000; pwr > 0; pwr /= 10) {
        digit = n / pwr;
        
        if(digit != 0 || flag) {
            c = (char)digit + '0';
            console_putc(c);
            
            flag = 1;
            n -= digit * pwr;
        }
    }    
}

void print_hex_nibble(unsigned char byte) {
    char c;
    
    c = byte & 0xf;
    if(c < 10) {
        c += '0';
    }
    else {
        c += ('a' - 10);
    }
    
    console_putc(c);
}

void print_hex_b(unsigned char byte) {
    print_hex_nibble( (char)byte );
    print_hex_nibble( (char)(byte>>4) );    
}

void print_hex_w(unsigned short word) {
    int off;
    
    for(off=16-4; off>=0; off-=4) {
        print_hex_nibble( (char)(word>>off) );
    }
}

void print_hex_l(unsigned long dword) {
    int off;
    
    for(off=32-4; off>=0; off-=4) {
        print_hex_nibble( (char)(dword>>off) );
    }
}

void print_hex_q(unsigned long long qword) {
    int off;
    
    for(off=64-4; off>=0; off-=4) {
        print_hex_nibble( (char)(qword>>off) );
    }
}
