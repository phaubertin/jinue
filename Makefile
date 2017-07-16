include header.mk

subdirs              = boot doc kernel proc $(libjinue)

kernel               = kernel/kernel
kernel_bin           = bin/kernel-bin
setup_boot           = boot/boot.o
setup16              = boot/setup.bin
kernel_img           = bin/jinue
jinue_iso            = bin/jinue.iso

temp_iso_fs          = bin/iso-tmp
grub_config          = boot/grub.cfg
grub_image_rel       = boot/grub/i386-pc/jinue.img
grub_image           = $(temp_iso_fs)/$(grub_image_rel)

symbols              = symbols.o symbols.c symbols.txt
target               = $(kernel_img)
unclean.extra        =  $(kernel_bin) \
                        $(symbols) \
                        $(setup_boot) \
                        $(setup16) \
                        $(jinue_iso)
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
	make -C doc

.PHONY: clean-doc
clean-doc:
	make -C doc clean	

# ----- kernel image
.PHONY: kernel
kernel:
	make -C kernel

$(kernel): kernel
	true

$(kernel_bin): $(kernel) proc symbols $(scripts)/kernel-bin.lds
	$(LINK.o) -Wl,-T,$(scripts)/kernel-bin.lds $(LOADLIBES) $(LDLIBS) -o $@

$(kernel_img): $(kernel_bin) $(setup_boot) $(setup16) $(scripts)/image.lds
	$(LINK.o) -Wl,-T,$(scripts)/image.lds $(LOADLIBES) $(LDLIBS) -o $@

# ----- process manager
.PHONY: proc
proc:
	make -C proc

# ----- kernel debugging symbols
.PHONY: symbols
symbols: symbols.o

symbols.txt: $(kernel)
	nm -n $< > $@

symbols.c: symbols.txt
	scripts/mksymbols.tcl $< $@

symbols.o: symbols.c

# ----- setup code, boot sector, etc.
.PHONY: boot
boot:
	make -C boot

$(setup_boot) $(setup16): boot

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
