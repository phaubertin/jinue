# Copyright (C) 2019 Philippe Aubertin.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 
# 3. Neither the name of the author nor the names of other contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

jinue_root = ..
include $(jinue_root)/header.mk

kernel_ldscript  = interface/i686/ld/kernel.ld
image_ldscript   = interface/i686/ld/image.ld
build_info_h     = build-info.gen.h

LDFLAGS         += -m32 -march=i686
KERNEL_LDFLAGS   = -T $(kernel_ldscript)
KERNEL_LDLIBS    = -lgcc
IMAGE_LDFLAGS    = -T $(image_ldscript)

c.lib            = $(libc)/libc-kernel.a
loader.bin       = $(loader)/loader

kernel_elf       = interface/i686/kernel
stripped         = $(kernel_elf)-stripped
image            = interface/i686/$(notdir $(kernel_img))
kmain			 = application/kmain.o
setup16          = interface/i686/setup16.o
setup32          = interface/i686/setup32.o
target           = $(image)

unclean.extra    = \
	$(kernel_elf) \
	$(build_info_h) \
	$(kernel_ldscript) \
	$(image_ldscript) \
	$(setup16) \
	$(setup32) \
	$(stripped)

sources.kernel.c = \
	application/syscalls/close.c \
	application/syscalls/create_endpoint.c \
	application/syscalls/create_process.c \
	application/syscalls/create_thread.c \
	application/syscalls/destroy.c \
	application/syscalls/dup.c \
	application/syscalls/exit_thread.c \
	application/syscalls/get_thread_local.c \
	application/syscalls/get_user_memory.c \
	application/syscalls/join_thread.c \
	application/syscalls/mint.c \
	application/syscalls/mmap.c \
	application/syscalls/mclone.c \
	application/syscalls/puts.c \
	application/syscalls/reboot.c \
	application/syscalls/receive.c \
	application/syscalls/reply.c \
	application/syscalls/send.c \
	application/syscalls/set_thread_local.c \
	application/syscalls/start_thread.c \
	application/syscalls/yield_thread.c \
	application/kmain.c \
	domain/alloc/page_alloc.c \
	domain/alloc/slab.c \
	domain/alloc/vmalloc.c \
	domain/entities/descriptor.c \
	domain/entities/endpoint.c \
	domain/entities/object.c \
	domain/entities/process.c \
	domain/entities/thread.c \
	domain/services/cmdline.c \
	domain/services/exec.c \
	domain/services/ipc.c \
	domain/services/logging.c \
	domain/services/panic.c \
	domain/config.c \
	infrastructure/i686/auxv.c \
	infrastructure/i686/boot_alloc.c \
	infrastructure/i686/cmdline.c \
	infrastructure/i686/config.c \
	infrastructure/i686/cpu.c \
	infrastructure/i686/debug.c \
	infrastructure/i686/halt.c \
	infrastructure/i686/init.c \
	infrastructure/i686/memory.c \
	infrastructure/i686/pic8259.c \
	infrastructure/i686/process.c \
	infrastructure/i686/thread.c \
	infrastructure/i686/uart16550a.c \
	infrastructure/i686/vga.c \
	infrastructure/i686/vm.c \
	infrastructure/i686/vm_pae.c \
	infrastructure/i686/vm_x86.c \
	infrastructure/elf.c \
	interface/i686/bootinfo.c \
	interface/i686/interrupt.c \
	interface/syscalls.c

sources.kernel.nasm = \
	infrastructure/i686/abi.asm \
	infrastructure/i686/io.asm \
	infrastructure/i686/remap.asm \
	infrastructure/i686/thread_switch.asm \
	infrastructure/i686/x86.asm \
	interface/i686/crt.asm \
	interface/i686/trap.asm

sources.setup.nasm = \
	interface/i686/setup16.asm \
	interface/i686/setup32.asm

sources.c = $(sources.kernel.c)
sources.nasm = $(sources.kernel.nasm) $(sources.setup.nasm)

objects.kernel = $(sources.kernel.nasm:%.asm=%.o) $(sources.kernel.c:%.c=%.o)

include $(common)

.PHONY: image
image: $(image)

.PHONY: kernel
kernel: $(kernel_elf)

.PHONY: kernel-stripped
kernel-stripped: $(stripped)

$(build_info_h): FORCE
	$(scripts)/gen_build_info_h.sh > $@
	
$(kmain): $(kmain:%.o=%.c) $(build_info_h)

$(setup16): $(setup16:%.o=%.asm)

$(setup32): $(setup32:%.o=%.asm)

$(kernel_elf): $(objects.kernel) $(kernel_ldscript) libc
	$(LINK.o) $(KERNEL_LDFLAGS) $(objects.kernel) $(c.lib) $(KERNEL_LDLIBS) -o $@

$(image): $(setup16) $(setup32) $(kernel_elf)-stripped loader $(image_ldscript)
	$(LINK.o) $(IMAGE_LDFLAGS) -o $@

.PHONY: libc
libc:
	$(MAKE) -C $(libc)

.PHONY: loader
loader:
	$(MAKE) -C $(loader)

$(c.lib): libc

$(loader.bin): loader
