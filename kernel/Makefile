includes = ../include
scripts  = ../scripts

OBJS = alloc.o assert.o boot.o kernel.o io.o panic.o printk.o slab.o startup.o \
	   vm.o vm_alloc.o vga.o

.PHONY: all
all: kernel

.PHONY: clean
clean:
	-rm -f kernel $(OBJS)

kernel: $(OBJS) $(scripts)/kernel.lds
	$(LD) -r -T $(scripts)/kernel.lds -o $@ $(OBJS)

alloc.o:    alloc.c     $(includes)/alloc.h
assert.o:   assert.c    $(includes)/assert.h
boot.o:     boot.c      $(includes)/boot.h
kernel.o:   kernel.c    $(includes)/kernel.h
io.o:       io.asm      $(includes)/io.h
panic.o:    panic.c     $(includes)/panic.h
printk.o:   printk.c    $(includes)/printk.h
slab.o:     slab.c      $(includes)/vm_alloc.h $(includes)/slab.h
startup.o:  startup.asm $(includes)/startup.h
vga.o:      vga.c       $(includes)/vga.h
vm.o:       vm.c        $(includes)/vm.h
vm_alloc.o: vm_alloc.c  $(includes)/vm_alloc.h $(includes)/slab.h

%.o: %.c
	$(CC) $(CFLAGS) -Wall -g3 -c -nostdinc -fno-builtin -I$(includes) -o $@ $<

%.o: %.asm
	nasm -f elf -o $@ $<

