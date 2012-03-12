includes = ../include
scripts  = ../scripts

OBJS = alloc.o assert.o boot.o bootmem.o cpu.o debug.o elf.o kbd.o \
       kernel.o interrupt.o ipc.o panic.o printk.o syscall.o thread.o \
       vga.o vm.o vm_alloc.o

KERNEL-ASM = asm/kernel-asm.o
STARTUP    = asm/startup.o

DEBUG   ?= yes

ifeq ($(DEBUG), yes)
	DEBUG_FLAGS = -g3
else
	DEBUG_FLAGS = -DNDEBUG
endif

CFLAGS += -O3 $(DEBUG_FLAGS) -nostdinc -ffreestanding -fno-common -fno-inline -fno-omit-frame-pointer -ansi -Wall -Werror=implicit-function-declaration -Werror=uninitialized

.PHONY: all
all: kernel

.PHONY: clean
clean:
	make -C asm clean
	-rm -f kernel $(OBJS)

kernel: $(STARTUP) $(OBJS) $(KERNEL-ASM) $(scripts)/kernel.lds
	$(LD) -r -T $(scripts)/kernel.lds -o $@ $(STARTUP) $(OBJS) $(KERNEL-ASM)

.PHONY: asm
asm:
	make DEBUG=$(DEBUG) -C asm

$(STARTUP) $(KERNEL-ASM): asm

alloc.o:       alloc.c       $(includes)/alloc.h
assert.o:      assert.c      $(includes)/kstdc/assert.h
boot.o:        boot.c        $(includes)/boot.h
bootmem.o:     bootmem.c     $(includes)/bootmem.h
cpu.o:         cpu.c         $(includes)/cpu.h
kbd.o:         kbd.c         $(includes)/kbd.h
kernel.o:      kernel.c      $(includes)/kernel.h
interrupt.o:   interrupt.c   $(includes)/interrupt.h
ipc.o:         ipc.c         $(includes)/ipc.h
panic.o:       panic.c       $(includes)/panic.h
printk.o:      printk.c      $(includes)/printk.h
syscall.o:     syscall.c     $(includes)/syscall.h
thread.o:      thread.c      $(includes)/thread.h
vga.o:         vga.c         $(includes)/vga.h
vm.o:          vm.c          $(includes)/vm.h
vm_alloc.o:    vm_alloc.c    $(includes)/vm_alloc.h

%.o: %.c
	$(CC) $(CFLAGS) -c -I$(includes) -I$(includes)/kstdc -o $@ $<
