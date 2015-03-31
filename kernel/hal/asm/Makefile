includes = ../../../include/

OBJS    = frame_pointer.o io.o irq.o syscall.o thread.o x86.o
STARTUP = startup.o
TARGET  = kern-hal-asm.o

.PHONY: all
all: $(TARGET) $(STARTUP)

.PHONY: clean
clean:
	-rm -f $(TARGET) $(STARTUP) $(OBJS)

$(TARGET): $(OBJS)
	$(LD) -r -m elf_i386 -o $@ $^

frame_pointer.o:    frame_pointer.asm	$(includes)/hal/frame_pointer.h
io.o:          		io.asm        		$(includes)/hal/io.h
irq.o:         		irq.asm       		$(includes)/hal/irq.h
startup.o:     		startup.asm   		$(includes)/hal/startup.h
syscall.o:     		syscall.asm   		$(includes)/hal/syscall.h
thread.o:      		thread.asm    		$(includes)/thread.h
x86.o:         		x86.asm       		$(includes)/hal/x86.h

%.o: %.asm
	nasm -f elf32 -o $@ $<
