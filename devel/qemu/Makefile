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

# GRUB location (for creation of bootable ISO for virtual machine)
#
# TODO this should probably be configurable through some sort of configure script
grub_modules         = /usr/lib/grub/i386-pc

temp_iso_fs          = iso-tmp
grub_config          = grub.cfg
jinue_iso            = jinue.iso
grub_image_rel       = boot/grub/i386-pc/jinue.img
grub_image           = $(temp_iso_fs)/$(grub_image_rel)
kernel_img_copy      = $(temp_iso_fs)/boot/$(notdir $(kernel_img))
initrd_copy          = $(temp_iso_fs)/boot/$(notdir $(testapp_initrd))
run_log              = run-jinue.log
check_script         = check-log.sh

target               = $(jinue_iso)
unclean_recursive    = $(temp_iso_fs)

include $(common)

.PHONY: run
run: $(jinue_iso)
	qemu-system-i386                                \
	-cpu $(QEMU_CPU)                                \
	-m $(QEMU_MEM)                                  \
	-no-reboot                                      \
	-drive format=raw,media=cdrom,file=$(jinue_iso) \
	-serial stdio                                   \
	-smp 1                                          \
	-usb                                            \
	-vga std

.PHONY: run-no-display
run-no-display: $(jinue_iso)
	qemu-system-i386                                \
	-cpu $(QEMU_CPU)                                \
	-m $(QEMU_MEM)                                  \
	-no-reboot                                      \
	-drive format=raw,media=cdrom,file=$(jinue_iso) \
	-serial stdio                                   \
	-display none                                   \
	-smp 1                                          \
	-usb                                            \
	-vga std

.PHONY: check
check: $(run_log)
	./$(check_script) $<

.PHONY: kernel-image
kernel-image:
	make -C $(jinue_root)/kernel

.PHONY: testapp-initrd
testapp-initrd:
	make -C $(testapp)

$(kernel_img): kernel-image

$(testapp_initrd): testapp-initrd

$(temp_iso_fs): $(grub_config)
	mkdir -p $(temp_iso_fs)/boot/grub/i386-pc
	cp $(grub_modules)/* $(temp_iso_fs)/boot/grub/i386-pc/
	cp $(grub_config) $(temp_iso_fs)/boot/grub/
	touch $(temp_iso_fs)

$(kernel_img_copy): $(kernel_img) $(temp_iso_fs) kernel-image
	cp $< $@

$(initrd_copy): $(testapp_initrd) $(temp_iso_fs) testapp-initrd
	cp $< $@

$(grub_image): $(initrd_copy) $(kernel_img_copy)
	`grub2-mkimage --version > /dev/null 2>&1 && echo grub2-mkimage || echo grub-mkimage` \
		--prefix '/boot/grub' \
		--output $(grub_image) \
		--format i386-pc-eltorito \
		--compression auto \
		--config $(grub_config) \
		biosdisk iso9660

$(jinue_iso): $(grub_image)
	xorriso \
		-as mkisofs \
		-graft-points \
		-b $(grub_image_rel) \
		-no-emul-boot \
		-boot-load-size 4 \
		-boot-info-table \
		--grub2-boot-info \
		--grub2-mbr \
		$(grub_modules)/boot_hybrid.img \
		--protective-msdos-label \
		-o $(jinue_iso) \
		-r $(temp_iso_fs) \
		--sort-weight 0 \
		/ \
		--sort-weight 1 \
		/boot

$(run_log): $(jinue_iso)
	qemu-system-i386                                \
	-cpu $(QEMU_CPU)                                \
	-m $(QEMU_MEM)                                  \
	-no-reboot                                      \
	-drive format=raw,media=cdrom,file=$(jinue_iso) \
	-serial stdio                                   \
	-display none                                   \
	-smp 1                                          \
	-usb                                            \
	-vga std | tee $@
