# Copyright (C) 2017 Philippe Aubertin.
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

subdirs			 = core debug hal kstdc mem

kernel_ldscript	 = $(scripts)/kernel.ld

LDFLAGS         += -T $(kernel_ldscript)

kernel.core   	 = core/kern-core.o
kernel.debug  	 = debug/kern-debug.o
kernel.hal    	 = hal/kern-hal.o
kernel.lib    	 = ../lib/jinue/libjinue-kernel.a
kernel.mem    	 = mem/kern-mem.o
kernel.stdc   	 = kstdc/kern-stdc.o

target	 		 = kernel-stripped
unclean.extra    = kernel
objects.kernel	 = \
	$(kernel.core) \
	$(kernel.debug) \
	$(kernel.hal) \
    $(kernel.lib) \
    $(kernel.mem) \
    $(kernel.stdc) \

include $(common)

.PHONY: scripts
scripts:
	$(MAKE) -C $(scripts)

$(kernel_ldscript): scripts

kernel: $(objects.kernel) $(kernel_ldscript)
	$(LINK.o) $(objects.kernel) $(LOADLIBES) $(LDLIBS) -o $@

$(target): kernel

.PHONY: core
core:
	$(MAKE) -C $@

.PHONY: debug
debug:
	$(MAKE) -C $@

.PHONY: hal
hal:
	$(MAKE) -C $@

.PHONY: kstdc
kstdc:
	$(MAKE) -C $@

.PHONY: mem
mem:
	$(MAKE) -C $@

.PHONY: libjinue
libjinue:
	$(MAKE) -C $(libjinue) all-kernel

# The core directory must be re-maked every build because of the auto-generated
# information about the build date and time.
$(kernel.core): core

$(kernel.debug): debug

$(kernel.hal): hal

$(kernel.lib): libjinue

$(kernel.mem): mem

$(kernel.stdc): kstdc
