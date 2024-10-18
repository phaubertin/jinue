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

#ifndef JINUE_KERNEL_INFRASTRUCTURE_I686_VM_H
#define JINUE_KERNEL_INFRASTRUCTURE_I686_VM_H

/** This header file contains the public interface of the low-level page table
 * management code located in hal/vm.c and hal/vm_pae.c. */

#include <jinue/shared/vm.h>
#include <kernel/infrastructure/i686/asm/vm.h>
#include <kernel/infrastructure/i686/types.h>
#include <kernel/interface/i686/asm/boot.h>
#include <kernel/interface/i686/types.h>

/** convert physical to virtual address for kernel loaded at 0x100000 (1MB) */
#define PHYS_TO_VIRT_AT_1MB(x)      (((uintptr_t)(x)) + BOOT_OFFSET_FROM_1MB)

/** convert virtual to physical address for kernel loaded at 0x100000 (1MB) */
#define VIRT_TO_PHYS_AT_1MB(x)      (((uintptr_t)(x)) - BOOT_OFFSET_FROM_1MB)

/** convert pointer to physical address for kernel loaded at 0x100000 (1MB) */
#define PTR_TO_PHYS_ADDR_AT_1MB(x)  ((kern_paddr_t)VIRT_TO_PHYS_AT_1MB(x))

/** convert physical to virtual address for kernel loaded at 0x1000000 (16MB) */
#define PHYS_TO_VIRT_AT_16MB(x)      (((uintptr_t)(x)) + BOOT_OFFSET_FROM_16MB)

/** convert virtual to physical address for kernel loaded at 0x1000000 (16MB) */
#define VIRT_TO_PHYS_AT_16MB(x)      (((uintptr_t)(x)) - BOOT_OFFSET_FROM_16MB)

/** convert pointer to physical address for kernel loaded at 0x1000000 (16MB) */
#define PTR_TO_PHYS_ADDR_AT_16MB(x)  ((kern_paddr_t)VIRT_TO_PHYS_AT_16MB(x))

#define ADDR_4GB    UINT64_C(0x100000000)

void vm_set_no_pae(void);

void vm_write_protect_kernel_image(const bootinfo_t *bootinfo);

addr_space_t *vm_create_initial_addr_space(
        const exec_file_t   *kernel,
        boot_alloc_t        *boot_alloc,
        const bootinfo_t    *bootinfo) ;

bool vm_create_addr_space(addr_space_t *addr_space);

void vm_destroy_addr_space(addr_space_t *addr_space);

void vm_switch_addr_space(addr_space_t *addr_space, cpu_data_t *cpu_data);

void vm_boot_map(void *addr, uint32_t paddr, int num_entries);

void vm_map_kernel_page(void *vaddr, kern_paddr_t paddr, int flags);

void vm_unmap_kernel_page(void *addr);

void vm_unmap_userspace(addr_space_t *addr_space, void *addr);

bool vm_clone_range(
        addr_space_t    *dest_addr_space,
        addr_space_t    *src_addr_space,
        addr_t           dest_addr,
        addr_t           src_addr,
        size_t           length,
        int              prot);

#endif

