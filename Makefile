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

include header.mk

subdirs              = boot doc kernel proc $(libjinue) $(scripts)

kernel               = kernel/kernel
setup16              = boot/setup16.o
setup32				 = boot/setup32.o
vbox_initrd			 = boot/vbox-initrd.gz
kernel_img           = bin/jinue
jinue_iso            = bin/jinue.iso
vbox_vm_name		 = Jinue

image_ldscript	 	 = $(scripts)/image.ld

temp_iso_fs          = bin/iso-tmp
grub_config          = boot/grub.cfg
grub_image_rel       = boot/grub/i386-pc/jinue.img
grub_image           = $(temp_iso_fs)/$(grub_image_rel)

target               = $(kernel_img)
unclean.extra        = $(jinue_iso) $(vbox_initrd)
unclean_recursive    = $(temp_iso_fs)


# ----- main targets
include $(common)

.PHONY: install
install: $(kernel_img)
	install -m644 $< /boot

.PHONY: vbox
vbox: $(jinue_iso) $(vbox_initrd)

.PHONY: vbox-debug
vbox-debug: vbox
	vboxmanage startvm $(vbox_vm_name) -E VBOX_GUI_DBG_AUTO_SHOW=true -E VBOX_GUI_DBG_ENABLED=true
	
.PHONY: vbox-run
vbox-run: vbox
	vboxmanage startvm $(vbox_vm_name)

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

$(kernel): kernel
	true

$(kernel_img): $(setup16) $(setup32) $(kernel) proc $(image_ldscript)
	$(LINK.o) -Wl,-T,$(image_ldscript) $(LOADLIBES) $(LDLIBS) -o $@

# ----- process manager
.PHONY: proc
proc:
	$(MAKE) -C proc

# ----- setup code, boot sector, etc.
.PHONY: boot
boot:
	$(MAKE) -C boot

$(setup16) $(setup32): boot

# ----- bootable ISO image for virtual machine
$(temp_iso_fs): $(kernel_img) $(grub_config) $(vbox_initrd) FORCE
	mkdir -p $(temp_iso_fs)/boot/grub/i386-pc
	cp $(grub_modules)/* $(temp_iso_fs)/boot/grub/i386-pc/
	cp $(kernel_img) $(temp_iso_fs)/boot/
	cp $(grub_config) $(temp_iso_fs)/boot/grub/
	cp $(vbox_initrd) $(temp_iso_fs)/boot/
	touch $(temp_iso_fs)

$(grub_image): $(temp_iso_fs)
	grub2-mkimage --prefix '/boot/grub' --output $(grub_image) --format i386-pc-eltorito --compression auto --config $(grub_config) biosdisk iso9660

$(jinue_iso): $(grub_image)
	xorriso -as mkisofs -graft-points -b $(grub_image_rel) -no-emul-boot -boot-load-size 4 -boot-info-table --grub2-boot-info --grub2-mbr $(grub_modules)/boot_hybrid.img --protective-msdos-label -o $(jinue_iso) -r $(temp_iso_fs) --sort-weight 0 / --sort-weight 1 /boot
	
# Fake initrd file for virtual machine
$(vbox_initrd):
	head -c 1m < /dev/urandom | gzip > $(vbox_initrd)
