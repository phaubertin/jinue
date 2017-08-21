/*
 * Copyright (C) 2017 Philippe Aubertin.
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

#include <jinue-common/asm/vm.h>
#include <hal/asm/boot.h>

OUTPUT_FORMAT("binary")

TARGET("binary")
INPUT("proc/proc-stripped")
INPUT("kernel/kernel-stripped")

TARGET("elf32-i386")
INPUT("boot/setup32.o")


SECTIONS {
    setup32 BOOT_SETUP32_ADDR : {
        boot/setup32.o
    }
    
    . = BOOT_SETUP32_ADDR + BOOT_SETUP32_SIZE;
    kernel_start = .;
    
    kernel : {
        kernel/kernel-stripped
        kernel_end = ABSOLUTE(.);
        
        . = ALIGN(PAGE_SIZE);
    }
    
    proc_start = .;
    
    proc : {
        proc/proc-stripped
        proc_end = ABSOLUTE(.);
        
        . = ALIGN(PAGE_SIZE);
    }
    
    kernel_top = .;
    
    kernel_size = kernel_end - kernel_start;
    proc_size   = proc_end   - proc_start;
}
