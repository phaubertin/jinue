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

sources.c       	 = tests/ipc.c debug.c testapp.c util.c
testapp         	 = testapp
stripped             = $(testapp)-stripped
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
LDFLAGS         += -Wl,-u,_start -m32 -march=i686
LDLIBS           = -lgcc

unclean.extra    = $(testapp) $(stripped)

include $(common)

$(libjinue)/libjinue-syscalls.a $(libjinue)/libjinue-util.a: FORCE
	$(MAKE) -C $(libjinue)
	
$(libc)/libc.a: FORCE
	$(MAKE) -C $(libc)

$(testapp): $(objects) $(libjinue)/libjinue-util.a $(libc)/libc.a $(libjinue)/libjinue-syscalls.a

$(stripped): $(testapp)

$(temp_ramdisk_fs): $(stripped)
	mkdir -p $(temp_ramdisk_fs)/sbin
	cp $(stripped) $(temp_ramdisk_fs)/sbin/init
	touch $(temp_ramdisk_fs)

$(ramdisk): $(temp_ramdisk_fs)
	tar -czvf $(ramdisk) -C $(temp_ramdisk_fs) sbin
