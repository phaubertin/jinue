# Copyright (C) 2022 Philippe Aubertin.
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

vm_name         	 = Jinue

# GRUB location (for creation of bootable ISO for virtual machine)
#
# TODO this should probably be configurable through some sort of configure script
grub_modules         = /usr/lib/grub/i386-pc

kernel_img           = $(jinue_root)/kernel/i686/jinue
temp_iso_fs          = iso-tmp
grub_config          = grub.cfg
jinue_iso            = jinue.iso
grub_image_rel       = boot/grub/i386-pc/jinue.img
grub_image           = $(temp_iso_fs)/$(grub_image_rel)
kernel_img_copy      = $(temp_iso_fs)/boot/$(notdir $(kernel_img))
initrd               = $(temp_iso_fs)/boot/vbox-initrd.gz

target               = $(jinue_iso)
unclean_recursive    = $(temp_iso_fs)

include $(common)

.PHONY: run
run: $(jinue_iso)
	vboxmanage startvm $(vm_name)

.PHONY: debug
debug: $(jinue_iso)
	vboxmanage startvm $(vm_name) -E VBOX_GUI_DBG_AUTO_SHOW=true -E VBOX_GUI_DBG_ENABLED=true

.PHONY: kernel-image
kernel-image:
	make -C $(jinue_root)/kernel image

$(kernel_img): kernel-image

$(temp_iso_fs): $(grub_config)
	mkdir -p $(temp_iso_fs)/boot/grub/i386-pc
	cp $(grub_modules)/* $(temp_iso_fs)/boot/grub/i386-pc/
	cp $(grub_config) $(temp_iso_fs)/boot/grub/
	touch $(temp_iso_fs)

$(initrd): $(temp_iso_fs)
	head -c 1m < /dev/urandom | gzip > $@

$(kernel_img_copy): $(kernel_img) $(temp_iso_fs) kernel-image
	cp $< $@

$(grub_image): $(initrd) $(kernel_img_copy)
	grub2-mkimage --prefix '/boot/grub' --output $(grub_image) --format i386-pc-eltorito --compression auto --config $(grub_config) biosdisk iso9660

$(jinue_iso): $(grub_image)
	xorriso -as mkisofs -graft-points -b $(grub_image_rel) -no-emul-boot -boot-load-size 4 -boot-info-table --grub2-boot-info --grub2-mbr $(grub_modules)/boot_hybrid.img --protective-msdos-label -o $(jinue_iso) -r $(temp_iso_fs) --sort-weight 0 / --sort-weight 1 /boot
