includes = ../include
kernel   = ../kernel
lib      = ../lib
libjinue = $(lib)/jinue
libproc  = $(lib)/proc

OBJS  = proc.o

CFLAGS  += -O3 -nostdinc -ffreestanding -static -ansi -Wall -Werror=implicit-function-declaration -Werror=uninitialized
LDFLAGS += -static -nostdlib -e main
LIBS    = -L$(libjinue) -L$(libproc) -lproc -ljinue

.PHONY: all
all: proc

.PHONY: clean
clean:
	-rm -f proc $(OBJS)

$(libjinue)/libjinue.a:
	make -C $(libjinue)

$(libproc)/libproc.a:
	make -C $(libproc)

proc: $(OBJS) $(libjinue)/libjinue.a $(libproc)/libproc.a
	$(LD) $(LDFLAGS) $(OBJS) -o $@ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c -I$(includes) -o $@ $<

%.o: %.asm
	nasm -f elf -o $@ $<

