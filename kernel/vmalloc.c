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

#include <hal/vm.h>
#include <assert.h>
#include <boot.h>
#include <page_alloc.h>
#include <panic.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <types.h>
#include <util.h>
#include <vmalloc.h>


/**
 * @file
 *
 * Virtual address allocator
 *
 * This allocator (specifically vmalloc()) is used to allocate a free virtual
 * address in the kernel's address space to which a page frame can be mapped.
 *
 * If you want to allocate a mapped page ready to use, use the page allocator
 * instead, i.e. page_alloc().
 *
 * */

/**
 * Allocate a page of virtual address space.
 *
 * @return address of allocated page or NULL if allocation failed
 *
 * */
addr_t vmalloc(void) {
    /* TODO implement this */
    panic("vmalloc(): not implemented");

    /* never reached */
    return NULL;
}

/**
 * Free a page of virtual address space.
 *
 * @param page the address of the page to free
 *
 * */
void vmfree(addr_t page) {
    /* TODO implement this */
    panic("vmfree(): not implemented");
}

/**
 * Check whether the specified page is in the region managed by the allocator.
 *
 * @param page the address of the page
 * @return true if it is in the region, false otherwise
 *
 * */
bool vmalloc_is_in_range(addr_t page) {
    return is_kernel_pointer(page);
}
