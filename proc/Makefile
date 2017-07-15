jinue_root = ..
include $(jinue_root)/header.mk

sources.c		 = proc.c
proc			 = proc
target			 = $(proc)-stripped

LIBS    		 = -L$(libjinue) -ljinue -ljinue-proc

# The linker might not link crt.o from libjinue.a, which is required to
# provide the _start entry point but otherwise contains no symbol that
# other object files are interested in. I have seen different behaviour
# in this regard on different machines with different versions of the
# linker. By explicitly forcing _start as an undefined symbol, we
# ensure the linker links the startup code.
LDFLAGS         += -Wl,-u,_start

unclean.extra    = $(proc)

include $(common)

$(libjinue)/libjinue.a $(libjinue)/libjinue-proc.a: FORCE
	make -C $(libjinue)

$(proc): $(objects) $(libjinue)/libjinue.a $(libjinue)/libjinue-proc.a

$(target): $(proc)
