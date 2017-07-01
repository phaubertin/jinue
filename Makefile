include header.mk

subdirs              = doc kernel proc $(libjinue)

kernel               = kernel/kernel
kernel_bin           = bin/kernel-bin
kernel_img           = bin/jinue
jinue_iso            = bin/jinue.iso

boot_h               = boot/boot.gen.h
temp_iso_fs          = bin/iso-tmp
grub_config          = boot/grub.cfg
grub_image_rel       = boot/grub/i386-pc/jinue.img
grub_image           = $(temp_iso_fs)/$(grub_image_rel)

symbols              = symbols.o symbols.c symbols.txt
unclean              =  $(kernel_bin) \
                        $(kernel_img) \
                        $(symbols) \
                        bin/setup \
                        bin/boot \
                        boot/setup.nasm \
                        boot/boot.nasm \
                        $(boot_h) \
                        $(jinue_iso)
unclean_recursive    = $(temp_iso_fs)
target               = $(jinue_iso)

# ----- main targets
include $(common)

.PHONY: install
install: $(kernel_img)
	install -m644 $< /boot

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

$(kernel_img): $(kernel_bin) bin/boot bin/setup $(scripts)/image.lds
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
bin/boot: boot/boot.asm $(boot_h)

bin/setup: boot/setup.asm

$(boot_h): $(kernel_bin) bin/setup
	scripts/boot_sector_inc.sh $@ bin/setup $(kernel_bin)

bin/%: boot/%.nasm
	nasm -f bin -Iboot/ -o $@ $<

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
