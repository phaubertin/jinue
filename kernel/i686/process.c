/*
 * Copyright (C) 2024 Philippe Aubertin.
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

#include <kernel/i686/cpu_data.h>
#include <kernel/i686/vm.h>
#include <kernel/machine/process.h>

void machine_switch_to_process(process_t *process) {
    vm_switch_addr_space(
            &process->addr_space,
            get_cpu_local_data());
}

bool machine_init_process(process_t *process) {
    return vm_create_addr_space(&process->addr_space);
}

void machine_destroy_process(process_t *process) {
    vm_destroy_addr_space(&process->addr_space);
}

void machine_map_kernel(void *vaddr, kern_paddr_t paddr, int flags) {
    vm_map_kernel(vaddr, paddr, flags);
}

void machine_unmap_kernel(void *addr) {
    vm_unmap_kernel(addr);
}

bool machine_map_userspace(process_t *process, void *vaddr, user_paddr_t paddr, int flags) {
    return vm_map_userspace(&process->addr_space, vaddr, paddr, flags);
}

void machine_unmap_userspace(process_t *process, void *addr) {
    vm_unmap_userspace(&process->addr_space, addr);
}

/* TODO merge this with machine_map_userspace() */
bool machine_mmap(process_t *process, void *vaddr, size_t length, user_paddr_t paddr, int prot) {
    addr_t addr = vaddr;

    for(size_t idx = 0; idx < length / PAGE_SIZE; ++idx) {
        /* TODO We should be able to optimize by not looking up the page table
         * for each entry. */
        if(!vm_map_userspace(&process->addr_space, addr, paddr, prot)) {
            return false;
        }

        addr += PAGE_SIZE;
        paddr += PAGE_SIZE;
    }

    return true;
}

bool machine_mclone(
        process_t   *dest_process,
        process_t   *src_process,
        void        *src_addr,
        void        *dest_addr,
        size_t       length,
        int          prot) {
    
    return vm_clone_range(
        &dest_process->addr_space,
        &src_process->addr_space,
        dest_addr,
        src_addr,
        length,
        prot
    );
}
