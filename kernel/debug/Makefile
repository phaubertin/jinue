includes = ../../include

OBJS   = assert.o debug.o kbd.o panic.o printk.o
TARGET = kern-debug.o

DEBUG   ?= yes

ifeq ($(DEBUG), yes)
	DEBUG_FLAGS = -g3
else
	DEBUG_FLAGS = -DNDEBUG
endif

CFLAGS += -O3 $(DEBUG_FLAGS) -nostdinc -ffreestanding -fno-common -fno-inline -fno-omit-frame-pointer -ansi -Wall -Werror=implicit-function-declaration -Werror=uninitialized -Werror=return-type

.PHONY: all
all: $(TARGET)

.PHONY: clean
clean:
	-rm -f $(TARGET) $(OBJS)

$(TARGET): $(OBJS)
	$(LD) -r -o $@ $^

assert.o:      assert.c      $(includes)/kstdc/assert.h
debug.o:       debug.c       $(includes)/debug.h
kbd.o:         kbd.c         $(includes)/kbd.h
panic.o:       panic.c       $(includes)/panic.h
printk.o:      printk.c      $(includes)/printk.h

%.o: %.c
	$(CC) $(CFLAGS) -c -I$(includes) -I$(includes)/kstdc -o $@ $<
