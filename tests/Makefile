# Copyright (C) 2024-2026 Philippe Aubertin.
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

# GRUB location (for creation of bootable ISO for ISO tests)
#
# TODO this should probably be configurable through some sort of configure script
grub_modules         = /usr/lib/grub/i386-pc

grub_image_rel      = boot/grub/i386-pc/jinue.img
run_script          = run-test.sh
run_log             = run-test.log
test_script         = run-all-tests.gen.sh

deps                = $(kernel_img) $(testapp_initrd)

# These tests are run through run-test.sh, which uses QEMU's -kernel and
# -initrd arguments to load the kernel and test app. Only a .sh file is
# needed for these tests.
tests = \
	test_486_too_old \
	test_acpi \
	test_boot_no_nx \
	test_boot_nx \
	test_boot_pentium \
	test_detect_qemu \
	test_ipc \
	test_loader_exit \
	test_mp \
	test_vga_text_80x25

# These tests are run through run-iso-test.sh, which invokes QEMU with an ISO
# file. This Makefile builds an ISO file for each of these tests that loads
# the kernel and test app using GRUB. This should be done only for the tests
# that need GRUB, for example to set a specific video mode. Each test needs a
# .sh test script and a .cfg GRUB configuration file.
iso_tests = \
	test_iso_boot_linux16 \
	test_iso_boot_linux32 \
	test_iso_vesa_640x480x32 \
	test_iso_vesa_800x600x32 \
	test_iso_vesa_1024x768x32 \
	test_iso_vga_text_80x25

logs    := $(foreach t,$(tests),$(t).log) $(foreach t,$(iso_tests),$(t).log)
isos    := $(foreach t,$(iso_tests),$(t).iso)
isodirs := $(foreach t,$(iso_tests),$(t).d)

target          	= $(test_script)
unclean.extra       = $(logs) $(isos)
unclean_recursive   = $(foreach t,$(iso_tests),$(t).d)

include $(common)

$(test_script): Makefile
	echo "#!/bin/bash -e" > $@
	echo "# Generated file - see Makefile" >> $@
	echo "" >> $@
	echo "source utils.sh" >> $@
	echo "" >> $@
	echo "fail_count=0" >> $@
	for t in $(tests); do \
		echo "" >> $@; \
		echo "echo =============================================================" >> $@; \
		echo "echo ==== TEST $$t" >> $@; \
		echo "echo =============================================================" >> $@; \
		echo "./$(run_script) -kernel $(kernel_img) $(testapp_initrd) $$t.sh $$t.log || fail_count=\$$((fail_count + 1))" >> $@; \
	done
	for t in $(iso_tests); do \
		echo "" >> $@; \
		echo "echo =============================================================" >> $@; \
		echo "echo ==== TEST $$t" >> $@; \
		echo "echo =============================================================" >> $@; \
		echo "./$(run_script) -iso $$t.iso $$t.sh $$t.log || fail_count=\$$((fail_count + 1))" >> $@; \
	done
	echo "" >> $@
	echo "echo =============================================================" >> $@
	echo "echo ==== SUMMARY" >> $@
	echo "echo =============================================================" >> $@
	echo "total=$(words $(tests) $(iso_tests))" >> $@
	echo "echo Total:    \$$total" >> $@
	echo "echo Success:  \$$((total - fail_count))" >> $@
	echo "echo Failures: \$$((fail_count))" >> $@
	echo '[ "$$fail_count" -gt "0" ] && report_fail' >> $@
	echo "report_success" >> $@
	chmod 755 $@

.PHONY: check
check: $(test_script) $(isos) $(deps)
	./$(test_script)

.PHONY: kernel-image
kernel-image:
	make -C $(jinue_root)/kernel

.PHONY: testapp-initrd
testapp-initrd:
	make -C $(testapp)
	
$(kernel_img): kernel-image

$(testapp_initrd): testapp-initrd

.PRECIOUS: $(isodirs)

%.d: %.cfg $(kernel_img) $(testapp_initrd)
	# Create directory hierarchy
	mkdir -p $@/boot/grub/i386-pc
	
	# Copy GRUB configuration
	cp $< $@/boot/grub/grub.cfg
	
	# Copy kernel image and test app initial RAM disk
	cp $(kernel_img) $@/boot/
	cp $(testapp_initrd) $@/boot/

	# Set up GRUB
	`grub2-mkimage --version > /dev/null 2>&1 && echo grub2-mkimage || echo grub-mkimage` \
		--prefix '/boot/grub' \
		--output $@/$(grub_image_rel) \
		--format i386-pc-eltorito \
		--compression auto \
		biosdisk iso9660 linux linux16

	# Mark directory as up to date
	touch $@

%.iso: %.d
	xorriso \
		-as mkisofs \
		-graft-points \
		-b $(grub_image_rel) \
		-no-emul-boot \
		-boot-load-size 4 \
		-boot-info-table \
		--grub2-boot-info \
		--grub2-mbr $(grub_modules)/boot_hybrid.img \
		--protective-msdos-label \
		-o $@ \
		-r $< \
		--sort-weight 0 / \
		--sort-weight 1 /boot
