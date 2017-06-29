jinue_root = ..
include $(jinue_root)/header.mk

C_SOURCES		 = proc.c
PROC			 = proc
TARGET			 = $(PROC)-stripped
LIBS    		 = -L$(libjinue) -ljinue -ljinue-proc

UNCLEAN			+= $(PROC)

include $(common)

$(libjinue)/libjinue.a $(libjinue)/libjinue-proc.a: FORCE
	make -C $(libjinue)

$(PROC): $(OBJS) $(libjinue)/libjinue.a $(libjinue)/libjinue-proc.a

$(TARGET): $(PROC)
