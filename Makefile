include header.mk

subdirs		 = doc kernel proc $(libjinue)

kernel     	 = kernel/kernel
kernel_bin 	 = bin/kernel-bin
kernel_img 	 = bin/jinue

boot_h	   	 = boot/boot.gen.h

symbols    	 = symbols.o symbols.c symbols.txt
unclean    	 = $(kernel_bin) $(kernel_img) $(symbols) bin/setup bin/boot \
	           boot/setup.nasm boot/boot.nasm $(boot_h)
target		 = $(kernel_img)

# ----- main targets
include $(common)

.PHONY: install
install: $(target)
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
