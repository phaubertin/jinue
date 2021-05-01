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

#ifndef JINUE_HAL_VM_H
#define JINUE_HAL_VM_H

/** This header file contains the public interface of the low-level page table
 * management code located in hal/vm.c and hal/vm_pae.c. */

#include <jinue-common/vm.h>
#include <hal/asm/boot.h>
#include <hal/asm/vm.h>
#include <types.h>

/** convert a physical address to a virtual address before the switch to the first address space */
#define EARLY_PHYS_TO_VIRT(x)   (((uintptr_t)(x)) + BOOT_KERNEL_OFFSET)

/** convert a virtual address to a physical address before the switch to the first address space */
#define EARLY_VIRT_TO_PHYS(x)   (((uintptr_t)(x)) - BOOT_KERNEL_OFFSET)

/** convert a pointer to a page frame address (early mappings) */
#define EARLY_PTR_TO_PHYS_ADDR(x)  ((kern_paddr_t)EARLY_VIRT_TO_PHYS(x))

#define ADDR_4GB    UINT64_C(0x100000000)

extern addr_space_t initial_addr_space;

void vm_boot_init(
        const boot_info_t   *boot_info,
        bool                 use_pae,
        cpu_data_t          *cpu_data,
        boot_alloc_t        *boot_alloc);

void vm_boot_postinit(const boot_info_t *boot_info, boot_alloc_t *boot_alloc, bool use_pae);

void vm_map_kernel(addr_t vaddr, kern_paddr_t paddr, int flags);

void vm_map_user(addr_space_t *addr_space, addr_t vaddr, user_paddr_t paddr, int flags);

void vm_unmap_kernel(addr_t addr);

void vm_unmap_user(addr_space_t *addr_space, addr_t addr);

kern_paddr_t vm_lookup_kernel_paddr(addr_t addr);

void vm_change_flags(addr_space_t *addr_space, addr_t addr, int flags);

void vm_map_early(addr_t vaddr, kern_paddr_t paddr, int flags);

addr_space_t *vm_create_addr_space(addr_space_t *addr_space);

addr_space_t *vm_create_initial_addr_space(
        bool             use_pae,
        boot_alloc_t    *boot_alloc);

void vm_destroy_addr_space(addr_space_t *addr_space);

void vm_switch_addr_space(addr_space_t *addr_space, cpu_data_t *cpu_data);

#endif

