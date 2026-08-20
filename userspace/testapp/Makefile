# Copyright (C) 2019-2023 Philippe Aubertin.
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

jinue_root = ../..
include $(jinue_root)/header.mk

sources.c = \
	server/handlers/map_anon.c \
	server/debug.c \
	server/elf.c \
	server/exec.c \
	server/mappings.c \
	server/ramdisk.c \
	server/server.c \
	server/utils.c \
	tests/abcd.c \
	tests/aes.c \
	tests/cancel_thread.c \
	tests/cancel_thread_async.c \
	tests/exit_thread.c \
	tests/ipc.c \
	tests/scroll.c \
	tests/signal.c \
	tests/sse.c \
	testapp.c \
	utils.c
sources.nasm = \
	tests/aes.asm \
	tests/sse.asm

objects.testapp = \
	tests/abcd.o \
	tests/aes.o \
	tests/aes-nasm.o \
	tests/cancel_thread.o \
	tests/cancel_thread_async.o \
	tests/exit_thread.o \
	tests/ipc.o \
	tests/scroll.o \
	tests/signal.o \
	tests/sse.o \
	tests/sse-nasm.o \
	testapp.o \
	utils.o

objects.server = \
	server/handlers/map_anon.o \
	server/debug.o \
	server/elf.o \
	server/exec.o \
	server/mappings.o \
	server/ramdisk.o \
	server/server.o \
	server/utils.o \
	utils.o

testapp         	 = testapp
server				 = test-server
temp_ramdisk_fs		 = ramdisk-tmp
ramdisk 			 = $(notdir $(testapp_initrd))

target          	 = $(ramdisk)
unclean_recursive	 = $(temp_ramdisk_fs)

# The linker might not link crt.o from libjinue.a, which is required to
# provide the _start entry point but otherwise contains no symbol that
# other object files are interested in. I have seen different behaviour
# in this regard on different machines with different versions of the
# linker. By explicitly forcing _start as an undefined symbol, we
# ensure the linker links the startup code.
LDFLAGS         += -Wl,-u,_start $(CFLAGS.arch)
LDLIBS           = -lgcc

unclean.extra    = $(testapp) $(server)

include $(common)

$(libjinue_syscalls) $(libjinue_utils): FORCE
	$(MAKE) -C $(libjinue)
	
$(libc)/libc.a $(libc)/libpthread.a: FORCE
	$(MAKE) -C $(libc)

$(testapp): $(objects.testapp) $(libjinue_utils) $(libc)/libpthread.a $(libc)/libc.a $(libjinue_syscalls)
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@

$(server): $(objects.server) $(libjinue_utils) $(libc)/libpthread.a $(libc)/libc.a $(libjinue_syscalls)
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@

$(temp_ramdisk_fs): $(testapp) $(server)
	mkdir -p $(temp_ramdisk_fs)/sbin
	cp $(server) $(temp_ramdisk_fs)/sbin/init
	cp $(testapp) $(temp_ramdisk_fs)/sbin/$(testapp)
	touch $(temp_ramdisk_fs)

$(ramdisk): $(temp_ramdisk_fs)
	tar -czvf $(ramdisk) -C $(temp_ramdisk_fs) sbin
