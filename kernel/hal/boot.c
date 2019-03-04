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
#include <hal/kernel.h>
#include <panic.h>
#include <printk.h>
#include <stddef.h>
#include <util.h>


const boot_info_t *boot_info;

bool boot_info_check(bool panic_on_failure) {
    /* This data structure is accessed early during the boot process, before
     * paging is enabled. What this means is that, if boot_info is NULL and we
     * dereference it, it does *not* cause a page fault or any other CPU
     * exception. */
    if(boot_info == NULL) {
        if(panic_on_failure) {
            panic("Boot information structure pointer is NULL.");
        }
        
        return false;
    }
    
    if(boot_info->setup_signature != BOOT_SETUP_MAGIC) {
        if(panic_on_failure) {
            panic("Bad setup header signature.");
        }
        
        return false;
    }
    
    return true;
}

const boot_info_t *get_boot_info(void) {
    return boot_info;
}

void boot_info_dump(void) {
    printk("Boot information structure:\n");
    printk("    kernel_start    %x  %u\n", boot_info->kernel_start   , boot_info->kernel_start    );
    printk("    kernel_size     %x  %u\n", boot_info->kernel_size    , boot_info->kernel_size     );
    printk("    proc_start      %x  %u\n", boot_info->proc_start     , boot_info->proc_start      );
    printk("    proc_size       %x  %u\n", boot_info->proc_size      , boot_info->proc_size       );
    printk("    image_start     %x  %u\n", boot_info->image_start    , boot_info->image_start     );
    printk("    image_top       %x  %u\n", boot_info->image_top      , boot_info->image_top       );
    printk("    e820_entries    %x  %u\n", boot_info->e820_entries   , boot_info->e820_entries    );
    printk("    e820_map        %x  %u\n", boot_info->e820_map       , boot_info->e820_map        );
    printk("    boot_heap       %x  %u\n", boot_info->boot_heap      , boot_info->boot_heap       );
    printk("    boot_end        %x  %u\n", boot_info->boot_end       , boot_info->boot_end        );
    printk("    page_table      %x  %u\n", boot_info->page_table     , boot_info->page_table      );
    printk("    page_directory  %x  %u\n", boot_info->page_directory , boot_info->page_directory  );
    printk("    setup_signature %x  %u\n", boot_info->setup_signature, boot_info->setup_signature );
}

void boot_heap_init(boot_heap_t *boot_heap, void *ptr) {
    boot_heap->ptr          = ptr;
    boot_heap->pushed_state = NULL;
}

void *boot_heap_alloc_size(boot_heap_t *boot_heap, size_t size, size_t align) {
    if(align != 0) {
        boot_heap->ptr = ALIGN_END(boot_heap->ptr, align);
    }

    void *ptr       = boot_heap->ptr;
    boot_heap->ptr  = (char *)boot_heap->ptr + size;

    return ptr;
}

void boot_heap_push(boot_heap_t *boot_heap) {
    struct boot_heap_pushed_state *pushed_state =
            boot_heap_alloc(boot_heap, struct boot_heap_pushed_state, 0);

    pushed_state->next      = boot_heap->pushed_state;
    boot_heap->pushed_state = pushed_state;
}

void boot_heap_pop(boot_heap_t *boot_heap) {
    if(boot_heap->pushed_state == NULL) {
        panic("No more boot heap pushed state to pop.");
    }

    boot_heap->ptr          = boot_heap->pushed_state;
    boot_heap->pushed_state = boot_heap->pushed_state->next;
}
