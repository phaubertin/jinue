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

#include <hal/boot.h>
#include <hal/vm.h>
#include <panic.h>
#include <stddef.h>

/* There is no extern declaration of this global variable in any header file but
 * it is set in startup.asm.  */
const boot_info_t *boot_info;

bool boot_info_check(bool panic_on_failure) {
    const char *error_description = NULL;

    /* This data structure is accessed early during the boot process, when the
     * first two megabytes of memory are still identity mapped. This means, if
     * boot_info is NULL and we dereference it, it does *not* cause a page fault
     * or any other CPU exception. */
    if(boot_info == NULL) {
        error_description = "Boot information structure pointer is NULL.";
    }
    else if(boot_info->setup_signature != BOOT_SETUP_MAGIC) {
        error_description = "Bad setup header signature.";
    }
    else if(page_offset_of(boot_info->image_start) != 0) {
        error_description = "Bad image alignment.";
    }
    else if(page_offset_of(boot_info->kernel_start) != 0) {
        error_description = "Bad kernel alignment.";
    }
    else {
        return true;
    }

    if(panic_on_failure) {
        panic(error_description);
    }

    return false;
}

const boot_info_t *get_boot_info(void) {
    return boot_info;
}
