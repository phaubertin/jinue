includes = ../include
scripts  = ../scripts

#OBJS = alloc.o assert.o boot.o bootmem.o kernel.o io.o panic.o printk.o \
#	    process.o slab.o startup.o vm.o vm_alloc.o vga.o x86.o

OBJS = alloc.o assert.o elf.o boot.o bootmem.o kbd.o kernel.o io.o \
       interrupt.o ipc.o irq.o panic.o printk.o startup.o syscall.o vga.o vm.o \
       x86.o

CFLAGS += -O3 -g3 -nostdinc -ffreestanding -fno-common -ansi -Wall -Werror=implicit-function-declaration -Werror=uninitialized

.PHONY: all
all: kernel

.PHONY: clean
clean:
	-rm -f kernel $(OBJS)

kernel: $(OBJS) $(scripts)/kernel.lds
	$(LD) -r -T $(scripts)/kernel.lds -o $@ $(OBJS)

alloc.o:       alloc.c       $(includes)/alloc.h
assert.o:      assert.c      $(includes)/assert.h
boot.o:        boot.c        $(includes)/boot.h
kbd.o:         kbd.c         $(includes)/kbd.h
kernel.o:      kernel.c      $(includes)/kernel.h
io.o:          io.asm        $(includes)/io.h
interrupt.o:   interrupt.c   $(includes)/interrupt.h
ipc.o:         ipc.c         $(includes)/ipc.h
irq.o:         irq.asm       $(includes)/irq.h
panic.o:       panic.c       $(includes)/panic.h
printk.o:      printk.c      $(includes)/printk.h
process.o:     process.c     $(includes)/process.h
slab.o:        slab.c        $(includes)/vm_alloc.h $(includes)/slab.h
startup.o:     startup.asm   $(includes)/startup.h
syscall.o:     syscall.asm   $(includes)/syscall.h
vga.o:         vga.c         $(includes)/vga.h
vm.o:          vm.c          $(includes)/vm.h
vm_alloc.o:    vm_alloc.c    $(includes)/vm_alloc.h $(includes)/slab.h
x86.o:         x86.asm       $(includes)/x86.h

%.o: %.c
	$(CC) $(CFLAGS) -c -I$(includes) -o $@ $<

%.o: %.asm
	nasm -f elf -o $@ $<

