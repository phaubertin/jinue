includes = ../../../include

hal_includes = $(includes)/hal

OBJS    = frame_pointer.o io.o irq.o syscall.o thread.o x86.o
STARTUP = startup.o
TARGET  = kern-hal-asm.o

CPPFLAGS = -I$(includes) -nostdinc

.PHONY: all
all: $(TARGET) $(STARTUP)

.PHONY: clean
clean:
	-rm -f $(TARGET) $(STARTUP) $(STARTUP:%.o=%.nasm) $(OBJS) $(OBJS:%.o=%.nasm)

$(TARGET): $(OBJS)
	$(LD) -r -m elf_i386 -o $@ $^

frame_pointer.o:    frame_pointer.asm   $(hal_includes)/frame_pointer.h
io.o:               io.asm              $(hal_includes)/io.h
irq.o:              irq.asm             $(hal_includes)/irq.h
startup.o:          startup.asm         $(hal_includes)/startup.h
syscall.o:          syscall.asm         $(hal_includes)/syscall.h
thread.o:           thread.asm          $(hal_includes)/thread.h
x86.o:              x86.asm             $(hal_includes)/x86.h

%.nasm: %.asm
	$(CPP) $(CPPFLAGS) -x assembler-with-cpp -o $@ $<

%.o: %.nasm
	nasm -f elf -o $@ $<
