# Copyright (C) 2022-2024 Philippe Aubertin.
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

QEMU_CPU            ?= core2duo
QEMU_MEM            ?= 128

CMDLINE             = \
    on_panic=reboot \
    serial_enable=yes \
    serial_dev=/dev/ttyS0 \
    DEBUG_DUMP_MEMORY_MAP=1 \
    DEBUG_DUMP_SYSCALL_IMPLEMENTATION=1 \
    DEBUG_DUMP_RAMDISK=1 \
    RUN_TEST_ABCD=1 \
    RUN_TEST_IPC=1

run_log              = run-jinue.log
deps                 = $(kernel_img) $(testapp_initrd)
targets.phony        = all-deps

include $(common)

.PHONY: all-deps
all-deps: $(deps)

.PHONY: run
run: $(deps)
	qemu-system-i386            \
	-cpu $(QEMU_CPU)            \
	-m $(QEMU_MEM)              \
	-no-reboot                  \
	-kernel $(kernel_img)       \
	-initrd $(testapp_initrd)   \
	-append "$(CMDLINE)"        \
	-serial stdio               \
	-smp 1                      \
	-usb                        \
	-vga std | tee $(run_log)

.PHONY: run-no-display
run-no-display: $(deps)
	qemu-system-i386                       \
	-cpu $(QEMU_CPU)                       \
	-m $(QEMU_MEM)                         \
	-no-reboot                             \
	-kernel $(kernel_img)                  \
	-initrd $(testapp_initrd)              \
	-append "$(CMDLINE) DEBUG_DO_REBOOT=1" \
	-serial stdio                          \
	-display none                          \
	-smp 1                                 \
	-usb                                   \
	-vga std | tee $(run_log)

.PHONY: debug
debug: $(deps)
	qemu-system-i386            \
	-cpu $(QEMU_CPU)            \
	-m $(QEMU_MEM)              \
	-no-reboot                  \
	-kernel $(kernel_img)       \
	-initrd $(testapp_initrd)   \
	-append "$(CMDLINE)"        \
	-serial stdio               \
	-smp 1                      \
	-usb                        \
	-s -S                       \
	-vga std | tee $(run_log)

.PHONY: debug-no-display
debug-no-display: $(deps)
	qemu-system-i386                       \
	-cpu $(QEMU_CPU)                       \
	-m $(QEMU_MEM)                         \
	-no-reboot                             \
	-kernel $(kernel_img)                  \
	-initrd $(testapp_initrd)              \
	-append "$(CMDLINE) DEBUG_DO_REBOOT=1" \
	-serial stdio                          \
	-display none                          \
	-smp 1                                 \
	-usb                                   \
	-s -S                                  \
	-vga std | tee $(run_log)

.PHONY: kernel-image
kernel-image:
	make -C $(jinue_root)/kernel

.PHONY: testapp-initrd
testapp-initrd:
	make -C $(testapp)

$(kernel_img): kernel-image

$(testapp_initrd): testapp-initrd

$(run_log): run-no-display
