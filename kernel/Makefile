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

kernel_ldscript  = i686/ld/kernel.ld
image_ldscript   = i686/ld/image.ld
build_info_h     = build-info.gen.h

LDFLAGS         += -m32 -march=i686
KERNEL_LDFLAGS   = -T $(kernel_ldscript) -static-libgcc
KERNEL_LDLIBS    = -lgcc
IMAGE_LDFLAGS    = -T $(image_ldscript)

c.lib            = $(libc)/libc-kernel.a
loader.bin       = $(loader)/loader

kernel           = i686/kernel
stripped         = i686/kernel-stripped
image            = i686/jinue
setup16          = i686/setup16.o
setup32          = i686/setup32.o
targets          = $(setup16) $(setup32) $(stripped)

unclean.extra    = \
	$(kernel) \
	$(image) \
	$(build_info_h) \
	$(kernel_ldscript) \
	$(image_ldscript) \
	$(setup16) \
	$(setup32)

sources.kernel.c = \
	i686/auxv.c \
	i686/boot.c \
	i686/boot_alloc.c \
	i686/cpu.c \
	i686/debug.c \
	i686/halt.c \
	i686/init.c \
	i686/interrupt.c \
	i686/memory.c \
	i686/pic8259.c \
	i686/process.c \
	i686/serial.c \
	i686/thread.c \
	i686/vga.c \
	i686/vm.c \
	i686/vm_pae.c \
	i686/vm_x86.c \
	cmdline.c \
	descriptor.c \
	elf.c \
	ipc.c \
	kmain.c \
	logging.c \
	object.c \
	page_alloc.c \
	panic.c \
	process.c \
	slab.c \
	syscall.c \
	thread.c \
	vmalloc.c
	
sources.kernel.nasm = \
	i686/abi.asm \
	i686/crt.asm \
	i686/io.asm \
	i686/remap.asm \
	i686/trap.asm \
	i686/thread_switch.asm \
	i686/x86.asm

sources.setup.nasm = \
	i686/setup16.asm \
	i686/setup32.asm

sources.c = $(sources.kernel.c)
sources.nasm = $(sources.kernel.nasm) $(sources.setup.nasm)

objects.kernel = $(sources.kernel.nasm:%.asm=%.o) $(sources.kernel.c:%.c=%.o)

include $(common)

.PHONY: image
image: $(image)

$(build_info_h): FORCE
	$(scripts)/gen_build_info_h.sh > $@
	
kmain.o: kmain.c $(build_info_h)

$(setup16): i686/setup16.asm

$(setup32): i686/setup32.asm

$(kernel): $(objects.kernel) $(kernel_ldscript) libc
	$(LINK.o) $(KERNEL_LDFLAGS) $(objects.kernel) $(c.lib) $(KERNEL_LDLIBS) -o $@

$(image): $(setup16) $(setup32) $(kernel)-stripped loader $(image_ldscript)
	$(LINK.o) $(IMAGE_LDFLAGS) -o $@

.PHONY: libc
libc:
	$(MAKE) -C $(libc)

.PHONY: loader
loader:
	$(MAKE) -C $(loader)

$(c.lib): libc

$(loader.bin): loader
