includes = ../include
scripts  = ../scripts

OBJS = assert.o bios.o kernel.o io.o panic.o printk.o startup.o vga.o

.PHONY: all
all: kernel

.PHONY: clean
clean:
	-rm -f kernel $(OBJS)

kernel: $(OBJS) $(scripts)/kernel.lds
	$(LD) -r -T $(scripts)/kernel.lds -o $@ $(OBJS)

assert.o:  assert.c    $(includes)/assert.h
bios.o:    bios.c      $(includes)/bios.h
kernel.o:  kernel.c    $(includes)/kernel.h
io.o:      io.asm      $(includes)/io.h
panic.o:   panic.c     $(includes)/panic.h
printk.o:  printk.c    $(includes)/printk.h
startup.o: startup.asm $(includes)/startup.h
vga.o:     vga.c       $(includes)/vga.h

%.o: %.c
	$(CC) $(CFLAGS) -g3 -c -nostdinc -fno-builtin -I$(includes) -o $@ $<

%.o: %.asm
	nasm -f elf -o $@ $<

