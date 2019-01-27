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
    
    va_start(ap, format);
    
    int colour = CONSOLE_DEFAULT_COLOR;
    const char *idx = format;
    
    while(1) {
        const char *anchor = idx;
        
        while( *idx != 0 && *idx != '%' ) {
            ++idx;
        }
        
        ptrdiff_t size = idx - anchor;
        
        if(size > 0) {
            console_printn(anchor, size, colour);
        }
        
        if(*idx == 0 || *(idx+1) == 0) {
            break;
        }
        
        ++idx;        
        
        switch( *idx ) {
        case '%':
            console_putc('%', colour);
            break;
        
        case 'c':
            console_putc((char)va_arg(ap, int), colour);
            break;
        
        case 'k':
            colour = va_arg(ap, int);
            break;

        case 'q':
            print_hex_q(va_arg(ap, unsigned long long), colour);
            break;
        
        case 's':
            console_print(va_arg(ap, const char *), colour);
            break;
        
        case 'u':
            print_unsigned_int(va_arg(ap, unsigned int), colour);
            break;
        
        case 'b':
            print_hex_b((unsigned char)va_arg(ap, unsigned long), colour);
            break;
        
        case 'w':
            print_hex_w((unsigned short)va_arg(ap, unsigned long), colour);
            break;
        
        case 'x':
            print_hex_l(va_arg(ap, unsigned long), colour);
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

void print_unsigned_int(unsigned int n, int colour) {
    unsigned int flag = 0;
    unsigned int pwr;
    unsigned int digit;
    char c;
    
    if(n == 0) {
        console_putc('0', colour);
        return;
    }
    
    for(pwr = 1000 * 1000 * 1000; pwr > 0; pwr /= 10) {
        digit = n / pwr;
        
        if(digit != 0 || flag) {
            c = (char)digit + '0';
            console_putc(c, colour);
            
            flag = 1;
            n -= digit * pwr;
        }
    }    
}

void print_hex_nibble(unsigned char byte, int colour) {
    char c;
    
    c = byte & 0xf;
    if(c < 10) {
        c += '0';
    }
    else {
        c += ('a' - 10);
    }
    
    console_putc(c, colour);
}

void print_hex_b(unsigned char byte, int colour) {
    print_hex_nibble((char)byte, colour);
    print_hex_nibble((char)(byte>>4), colour);
}

void print_hex_w(unsigned short word, int colour) {
    int off;
    
    for(off=16-4; off>=0; off-=4) {
        print_hex_nibble((char)(word>>off), colour);
    }
}

void print_hex_l(unsigned long dword, int colour) {
    int off;
    
    for(off=32-4; off>=0; off-=4) {
        print_hex_nibble((char)(dword>>off), colour);
    }
}

void print_hex_q(unsigned long long qword, int colour) {
    int off;
    
    for(off=64-4; off>=0; off-=4) {
        print_hex_nibble((char)(qword>>off), colour);
    }
}
