includes = ../include
kernel   = ../kernel
lib      = ../lib
libjinue = $(lib)/jinue
libproc  = $(lib)/proc

OBJS  = proc.o

DEBUG   ?= yes

ifeq ($(DEBUG), yes)
	DEBUG_FLAGS = -g3
else
	DEBUG_FLAGS = -DNDEBUG
endif

CFLAGS  += -O3 $(DEBUG_FLAGS) -nostdinc -ffreestanding -static -ansi -Wall -Werror=implicit-function-declaration -Werror=uninitialized
LDFLAGS += -static -nostdlib -e main
LIBS    = -L$(libjinue) -L$(libproc) -lproc -ljinue

.PHONY: all
all: proc-stripped

.PHONY: clean
clean:
	-rm -f proc proc-stripped $(OBJS)

$(libjinue)/libjinue.a:
	make -C $(libjinue)

$(libproc)/libproc.a:
	make -C $(libproc)

proc: $(OBJS) $(libjinue)/libjinue.a $(libproc)/libproc.a
	$(LD) $(LDFLAGS) $(OBJS) -o $@ $(LIBS)

proc-stripped: proc
	strip --strip-debug -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -I$(includes) -I$(includes)/kstdc -o $@ $<

%.o: %.asm
	nasm -f elf -o $@ $<

