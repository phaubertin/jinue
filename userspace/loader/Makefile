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

sources.c        = \
	archives/alloc.c \
	archives/tar.c \
	binfmt/elf.c \
	streams/gzip.c \
	streams/raw.c \
	streams/stream.c \
	debug.c \
	loader.c \
	ramdisk.c \
	util.c
loader           = loader
target           = $(loader)-stripped

CPPFLAGS        += -I$(zlib) -DZLIB_CONST

# The linker might not link crt.o from libjinue.a, which is required to
# provide the _start entry point but otherwise contains no symbol that
# other object files are interested in. I have seen different behaviour
# in this regard on different machines with different versions of the
# linker. By explicitly forcing _start as an undefined symbol, we
# ensure the linker links the startup code.
LDFLAGS         += -Wl,-u,_start -m32 -march=i686
LDLIBS           = -lgcc

unclean.extra    = $(loader)

include $(common)

$(libjinue)/libjinue-syscalls.a $(libjinue)/libjinue-util.a: FORCE
	$(MAKE) -C $(libjinue)
	
$(libc)/libc.a: FORCE
	$(MAKE) -C $(libc)

$(zlib)/libz.a: FORCE
	$(MAKE) CC=$(CC) -C $(zlib) libz.a

$(bzip2)/libbz2.a: FORCE
	$(MAKE) CC=$(CC) -C $(bzip2) libbz2.a

$(loader): $(objects) $(libjinue)/libjinue-util.a $(zlib)/libz.a $(bzip2)/libbz2.a $(libc)/libc.a $(libjinue)/libjinue-syscalls.a

$(target): $(loader)
