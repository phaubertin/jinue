includes = ../../include
scripts  = ../../scripts

OBJS = boot.o bootmem.o cpu.o e820.o hal.o interrupt.o vga.o vm.o vm_x86.o

HAL-ASM     = asm/kern-hal-asm.o
HAL-STARTUP = asm/startup.o
TARGET      = kern-hal.o

DEBUG   ?= yes

ifeq ($(DEBUG), yes)
	DEBUG_FLAGS = -g3
else
	DEBUG_FLAGS = -DNDEBUG
endif

CFLAGS += -m32 -march=i686 -O3 $(DEBUG_FLAGS) -nostdinc -ffreestanding -fno-common -fno-inline -fno-omit-frame-pointer -ansi -Wall -Werror=implicit-function-declaration -Werror=uninitialized -Werror=return-type

.PHONY: all
all: $(TARGET)

.PHONY: clean
clean:
	make -C asm clean
	-rm -f $(TARGET) $(OBJS)
	
.PHONY: asm
asm:
	make DEBUG=$(DEBUG) -C asm

.PHONY: hal-startup
hal-startup : $(HAL-STARTUP)

$(HAL-STARTUP) $(HAL-ASM): asm

$(TARGET): $(OBJS) $(HAL-ASM)
	$(LD) -r -m elf_i386 -o $@ $^


boot.o:        boot.c        $(includes)/hal/boot.h
bootmem.o:     bootmem.c     $(includes)/hal/bootmem.h
cpu.o:         cpu.c         $(includes)/hal/cpu.h
e820.o:        e820.c        $(includes)/hal/e820.h
hal.o:         hal.c         $(includes)/hal/kernel.h
interrupt.o:   interrupt.c   $(includes)/hal/interrupt.h
vga.o:         vga.c         $(includes)/hal/vga.h
vm.o:          vm.c          $(includes)/hal/vm.h
vm_x86.o:      vm_x86.c      $(includes)/hal/vm_x86.h

%.o: %.c
	$(CC) $(CFLAGS) -c -I$(includes)/hal -I$(includes) -I$(includes)/kstdc -o $@ $<
