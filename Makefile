kernel     = kernel/kernel
kernel_bin = bin/kernel-bin
kernel_img = bin/jinue

unclean    = $(kernel_bin) $(kernel_img) bin/setup bin/boot boot/boot.inc

# ----- main targets
.PHONY: all
all: $(kernel_img)

.PHONY: install
install: $(kernel_img)
	install -m644 $(kernel_img) /boot

.PHONY: clean
clean:
	make -C kernel clean
	-rm -f $(unclean)

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

$(kernel_bin): $(kernel) proc
	$(LD) -T scripts/kernel-bin.lds -o $@

$(kernel_img): $(kernel_bin) bin/boot bin/setup
	$(LD) -T scripts/image.lds -o $@

# ----- process manager
.PHONY: proc
proc:
	make -C proc

# ----- setup code, boot sector, etc.
bin/boot: boot/boot.asm boot/boot.inc
	nasm -f bin -Iboot/ -o $@ $<

bin/setup: boot/setup.asm
	nasm -f bin -o $@ $<

boot/boot.inc: $(kernel_bin) bin/setup
	scripts/boot_sector_inc.sh boot/boot.inc bin/setup $(kernel_bin)

	
