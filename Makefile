include header.mk

subdirs              = boot doc kernel proc $(libjinue) $(scripts)

kernel               = kernel/kernel
kernel_bin           = bin/kernel-bin
setup_boot           = boot/boot.o
setup16              = boot/setup.bin
setup32				 = boot/setup32.o
kernel_img           = bin/jinue
jinue_iso            = bin/jinue.iso

kernel_bin_ldscript	 = $(scripts)/kernel-bin.ld
image_ldscript	 	 = $(scripts)/image.ld

temp_iso_fs          = bin/iso-tmp
grub_config          = boot/grub.cfg
grub_image_rel       = boot/grub/i386-pc/jinue.img
grub_image           = $(temp_iso_fs)/$(grub_image_rel)

target               = $(kernel_img)
unclean.extra        = $(kernel_bin) $(jinue_iso)
unclean_recursive    = $(temp_iso_fs)


# ----- main targets
include $(common)

.PHONY: install
install: $(kernel_img)
	install -m644 $< /boot

.PHONY: vbox
vbox: $(jinue_iso)

# ----- documentation
.PHONY: doc
doc:
	$(MAKE) -C doc

.PHONY: clean-doc
clean-doc:
	$(MAKE) -C doc clean

# ----- kernel image
.PHONY: scripts
scripts:
	$(MAKE) -C $(scripts)

.PHONY: kernel
kernel:
	$(MAKE) -C kernel

$(kernel_bin_ldscript) $(image_ldscript): scripts

$(kernel): kernel
	true

$(kernel_bin): $(setup32) $(kernel) proc $(kernel_bin_ldscript)
	$(LINK.o) -Wl,-T,$(kernel_bin_ldscript) $(LOADLIBES) $(LDLIBS) -o $@

$(kernel_img): $(kernel_bin) $(setup_boot) $(setup16) $(image_ldscript)
	$(LINK.o) -Wl,-T,$(image_ldscript) $(LOADLIBES) $(LDLIBS) -o $@

# ----- process manager
.PHONY: proc
proc:
	$(MAKE) -C proc

# ----- setup code, boot sector, etc.
.PHONY: boot
boot:
	$(MAKE) -C boot

$(setup_boot) $(setup16) $(setup32): boot

# ----- bootable ISO image for virtual machine
$(temp_iso_fs): $(kernel_img) $(grub_config) FORCE
	mkdir -p $(temp_iso_fs)/boot/grub/i386-pc
	cp $(grub_modules)/* $(temp_iso_fs)/boot/grub/i386-pc/
	cp $(kernel_img) $(temp_iso_fs)/boot/
	cp $(grub_config) $(temp_iso_fs)/boot/grub/
	touch $(temp_iso_fs)

$(grub_image): $(temp_iso_fs)
	grub2-mkimage --prefix '/boot/grub' --output $(grub_image) --format i386-pc-eltorito --compression auto --config $(grub_config) biosdisk iso9660

$(jinue_iso): $(grub_image)
	xorriso -as mkisofs -graft-points -b $(grub_image_rel) -no-emul-boot -boot-load-size 4 -boot-info-table --grub2-boot-info --grub2-mbr $(grub_modules)/boot_hybrid.img --protective-msdos-label -o $(jinue_iso) -r $(temp_iso_fs) --sort-weight 0 / --sort-weight 1 /boot
