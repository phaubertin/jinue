includes = ../include
scripts  = ../scripts

OBJS = bios.o kernel.o io.o printk.o startup.o vga.o

.PHONY: all
all: kernel

.PHONY: clean
clean:
	-rm -f kernel $(OBJS)

kernel: $(OBJS) $(scripts)/kernel.lds
	$(LD) -r -T $(scripts)/kernel.lds -o $@ $(OBJS)

bios.o:    bios.c      $(includes)/bios.h
kernel.o:  kernel.c    $(includes)/kernel.h
io.o:      io.asm      $(includes)/io.h
printk.o:  printk.c    $(includes)/printk.h
startup.o: startup.asm $(includes)/startup.h
vga.o:     vga.c       $(includes)/vga.h

%.o: %.c
	$(CC) $(CFLAGS) -g3 -c -nostdinc -fno-builtin -I$(includes) -o $@ $<

%.o: %.asm
	nasm -f elf -o $@ $<

