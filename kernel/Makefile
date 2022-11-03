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

kernel_ldscript	 = $(scripts)/kernel.ld
build_info_h     = build-info.gen.h

LDFLAGS         += -T $(kernel_ldscript) -static-libgcc -m32 -march=i686
LDLIBS           = -lgcc

kernel.i686    	 = i686/kern-i686.o
c.lib            = $(libc)/libc-kernel.a

target	 		 = kernel-stripped
unclean.extra    = kernel $(build_info_h)
sources.c		 = \
	i686/boot.c \
	i686/cpu.c \
	i686/interrupt.c \
	i686/machine.c \
	i686/memory.c \
	i686/pic8259.c \
	i686/serial.c \
	i686/thread.c \
	i686/vga.c \
	i686/vm.c \
	i686/vm_pae.c \
	i686/vm_x86.c \
	boot.c \
	cmdline.c \
	debug.c \
	elf.c \
	ipc.c \
	kmain.c \
	logging.c \
	page_alloc.c \
	panic.c \
	process.c \
	slab.c \
	syscall.c \
	thread.c \
	vmalloc.c
	
sources.nasm = \
	i686/abi.asm \
	i686/crt.asm \
	i686/io.asm \
	i686/remap.asm \
	i686/trap.asm \
	i686/thread_switch.asm \
	i686/x86.asm

objects.extra = $(c.lib)

include $(common)

$(build_info_h): FORCE
	$(scripts)/gen_build_info_h.sh > $@
	
kmain.o: kmain.c $(build_info_h)

kernel: $(objects) $(kernel_ldscript)
	$(LINK.o) $(objects) $(LOADLIBES) $(LDLIBS) -o $@

$(target): kernel

.PHONY: libc
libc:
	$(MAKE) -C $(libc)

$(c.lib): libc
