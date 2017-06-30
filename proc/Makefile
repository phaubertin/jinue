jinue_root = ..
include $(jinue_root)/header.mk

sources.c		 = proc.c
proc			 = proc
target			 = $(proc)-stripped

LIBS    		 = -L$(libjinue) -ljinue -ljinue-proc

unclean			+= $(proc)

include $(common)

$(libjinue)/libjinue.a $(libjinue)/libjinue-proc.a: FORCE
	make -C $(libjinue)

$(proc): $(objects) $(libjinue)/libjinue.a $(libjinue)/libjinue-proc.a

$(target): $(proc)
