jinue_root = ..
include $(jinue_root)/header.mk

UNCLEAN		 	 = latex html
PHONY_TARGETS	 = pdf

include $(common)

.PHONY: doc
doc:
	doxygen

.PHONY: pdf
pdf: doc
	make -C latex pdf
	cp latex/refman.pdf pdf/kernel-source-documentation.pdf
	
