# Copyright (C) 2019-2022 Philippe Aubertin.
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

include header.mk

subdirs              = doc kernel $(userspace)/loader $(libc) $(libjinue) $(virtualbox)

# ----- main targets
include $(common)

.PHONY: install
install: $(kernel_img) kernel-image
	install -m644 $< /boot

.PHONY: vbox
vbox: $(kernel_img)
	make -C $(virtualbox)

.PHONY: vbox-debug
vbox-debug: $(kernel_img)
	make -C $(virtualbox) debug
	
.PHONY: vbox-run
vbox-run: $(kernel_img)
	make -C $(virtualbox) run

.PHONY: cppcheck-kernel
cppcheck-kernel:
	cppcheck --enable=all --std=c99 --platform=unix32 $(CPPFLAGS.includes) $(kernel)

# ----- documentation
.PHONY: doc
doc:
	$(MAKE) -C doc

.PHONY: clean-doc
clean-doc:
	$(MAKE) -C doc clean

# ----- kernel image
.PHONY: kernel
kernel:
	$(MAKE) -C kernel

.PHONY: image
image:
	$(MAKE) -C kernel image

# ----- user space loader
.PHONY: loader
loader:
	$(MAKE) -C $(userspace)/loader
