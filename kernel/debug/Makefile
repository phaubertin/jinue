jinue_root = ../..
include $(jinue_root)/header.mk

OBJS   = debug.o kbd.o panic.o
TARGET = kern-debug.o

.PHONY: all
all: $(TARGET)

.PHONY: clean
clean:
	-rm -f $(TARGET) $(OBJS)

$(TARGET): $(OBJS)
	$(LINK.o) -r $^ $(LOADLIBES) $(LDLIBS) -o $@

debug.o:       debug.c       $(includes)/debug.h
kbd.o:         kbd.c         $(includes)/kbd.h
panic.o:       panic.c       $(includes)/panic.h

include $(jinue_root)/footer.mk
