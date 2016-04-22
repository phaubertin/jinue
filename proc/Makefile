includes = ../include
kernel   = ../kernel
lib      = ../lib
libjinue = $(lib)/jinue

OBJS  = proc.o

DEBUG   ?= yes

ifeq ($(DEBUG), yes)
	DEBUG_FLAGS = -g3
else
	DEBUG_FLAGS = -DNDEBUG
endif

CFLAGS  += -m32 -march=i686 -O3 $(DEBUG_FLAGS) -nostdinc -ffreestanding -static -std=c99 -pedantic -Wall -Werror=implicit-function-declaration -Werror=uninitialized -Werror=return-type
LDFLAGS += -m elf_i386 -static -nostdlib -e main
LIBS    = -L$(libjinue) -ljinue -ljinue-proc

.PHONY: all
all: proc-stripped

.PHONY: clean
clean:
	-rm -f proc proc-stripped $(OBJS)

$(libjinue)/libjinue.a $(libjinue)/libjinue-proc.a:
	make -C $(libjinue)

proc: $(OBJS) $(libjinue)/libjinue.a $(libjinue)/libjinue-proc.a
	$(LD) $(LDFLAGS) $(OBJS) -o $@ $(LIBS)

proc-stripped: proc
	strip --strip-debug -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -I$(includes) -I$(includes)/kstdc -o $@ $<

%.o: %.asm
	nasm -f elf -o $@ $<
