includes   = include/
kernel     = kernel/kernel
kernel_bin = bin/kernel-bin
kernel_img = bin/jinue

symbols    = symbols.o symbols.c symbols.txt
unclean    = $(kernel_bin) $(kernel_img) $(symbols) bin/setup bin/boot \
	         boot/setup.nasm boot/boot.nasm boot/boot.h

CPPFLAGS   = -I$(includes) -nostdinc

DEBUG     ?= yes

# ----- main targets
.PHONY: all
all: $(kernel_img)

.PHONY: install
install: $(kernel_img)
	install -m644 $(kernel_img) /boot

.PHONY: clean
clean:
	make -C kernel clean
	make -C proc clean
	make -C lib/jinue clean
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
	make DEBUG=$(DEBUG) -C kernel

$(kernel): kernel
	true

$(kernel_bin): $(kernel) proc symbols
	$(LD) -m elf_i386 -T scripts/kernel-bin.lds -o $@

$(kernel_img): $(kernel_bin) bin/boot bin/setup
	$(LD) -m elf_i386 -T scripts/image.lds -o $@

# ----- process manager
.PHONY: proc
proc:
	make DEBUG=$(DEBUG) -C proc

# ----- kernel debugging symbols
.PHONY: symbols
symbols: symbols.o

symbols.txt: $(kernel)
	nm -n $< > $@

symbols.c: symbols.txt
	scripts/mksymbols.tcl $< $@

symbols.o: symbols.c
	$(CC) -c -m32 -nostdinc -ffreestanding -fno-common -std=c99 -pedantic -Wall -Iinclude -Iinclude/kstdc -o $@ $<

# ----- setup code, boot sector, etc.
bin/boot: boot/boot.asm boot/boot.h

bin/setup: boot/setup.asm

boot/boot.h: $(kernel_bin) bin/setup
	scripts/boot_sector_inc.sh $@ bin/setup $(kernel_bin)

%.nasm: %.asm
	$(CPP) $(CPPFLAGS) -x assembler-with-cpp -o $@ $<

bin/%: boot/%.nasm
	nasm -f bin -Iboot/ -o $@ $<
