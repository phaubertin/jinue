jinue_root = ..
include $(jinue_root)/header.mk

kernel   = $(jinue_root)/kernel

OBJS  = proc.o

LIBS    	 = -L$(libjinue) -ljinue -ljinue-proc

.PHONY: all
all: proc-stripped

.PHONY: clean
clean:
	-rm -f proc proc-stripped $(OBJS)

$(libjinue)/libjinue.a $(libjinue)/libjinue-proc.a: FORCE
	make -C $(libjinue)

proc: $(OBJS) $(libjinue)/libjinue.a $(libjinue)/libjinue-proc.a

proc-stripped: proc

include $(jinue_root)/footer.mk
