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

subdirs = \
	doc \
	kernel \
	$(userspace)/loader \
	$(userspace)/testapp \
	$(libc) \
	$(libjinue) \
	$(zlib) \
	$(bzip2) \
	$(qemu) \
	$(virtualbox)

# make all (default target) will build the kernel image
targets.phony = kernel

# main targets
include $(common)

# install kernel image in /boot
.PHONY: install
install: $(kernel_img)
	install -m644 $< /boot

# install testapp ramdisk in /boot
.PHONY: install-testapp
install-testapp: $(testapp_initrd)
	install -m644 $< /boot

# build the ISO file (CDROM image) needed by the VirtualBox VM
.PHONY: vbox
vbox:
	make -C $(virtualbox)

# build the ISO file for VirtualBox and run the VM with the debugger
.PHONY: vbox-debug
vbox-debug:
	make -C $(virtualbox) debug

# build the ISO file for VirtualBox and run the VM (without the debugger)
.PHONY: vbox-run
vbox-run:
	make -C $(virtualbox) run
	
# build the ISO file (CDROM image) needed by the QEMU
.PHONY: qemu
qemu:
	make -C $(qemu)

# build the ISO file for QEMU and run
.PHONY: qemu-run
qemu-run:
	make -C $(qemu) run

# build the ISO file for QEMU and run (without the debugger)
.PHONY: qemu-run-no-display
qemu-run-no-display:
	make -C $(qemu) run-no-display

# Run cppcheck on the kernel sources
# Note: there are known failures
.PHONY: cppcheck-kernel
cppcheck-kernel:
	cppcheck --enable=all --std=c99 --platform=unix32 $(CPPFLAGS.includes) $(kernel)

# build the Doxygen documentation
# (not really maintained)
.PHONY: doc
doc:
	$(MAKE) -C doc

# clean the Doxygen docuumentation
.PHONY: clean-doc
clean-doc:
	$(MAKE) -C doc clean

# build the kernel image (set up code + kernel ELF + user space loader ELF)
$(kernel_img): kernel

.PHONY: kernel
kernel:
	$(MAKE) -C kernel

# build the user space loader
.PHONY: loader
loader:
	$(MAKE) -C $(userspace)/loader

# build the test application
$(testapp_initrd): testapp

.PHONY: testapp
testapp:
	$(MAKE) -C $(userspace)/testapp
