includes = ../../include

OBJS    = frame_pointer.o io.o irq.o syscall.o thread.o x86.o
STARTUP = startup.o
TARGET  = kernel-asm.o

.PHONY: all
all: $(TARGET) $(STARTUP)

.PHONY: clean
clean:
	-rm -f $(TARGET) $(STARTUP) $(OBJS)

$(TARGET): $(OBJS)
	$(LD) -r -o $@ $(OBJS)

frame_pointer.o:    frame_pointer.asm	$(includes)/frame_pointer.h
io.o:          		io.asm        		$(includes)/io.h
irq.o:         		irq.asm       		$(includes)/irq.h
startup.o:     		startup.asm   		$(includes)/startup.h
syscall.o:     		syscall.asm   		$(includes)/syscall.h
thread.o:      		thread.asm    		$(includes)/thread.h
x86.o:         		x86.asm       		$(includes)/x86.h

%.o: %.asm
	nasm -f elf -o $@ $<
