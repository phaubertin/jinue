# Each Makefile is expected to:
#
#    1) Set the jinue_root variable to the path of the directory that contains
#       that Makefile relative to the top directory of the project (i.e. the one
#       that contains this file).
#
#   2) Include this file at the top of the Makefile.
#
#   3) Have the following statement at the point in the Makefile where the first
#      make target (i.e. all) should be included:
#           include $(common)
#
# This file defines default flags used when calling the various build tools as
# well as other useful variables. Any variable defined in this file can be
# overwritten or added to by each individual Makefile if needed.
#
# The $(common) variable is defined in this file and refers to common.mk, which
# is a Make include file that contains definitions for the generic targets
# (all, clean), implicit rules, as well as other useful definitions.

# relative path to the root of the project
#
# Makefiles that include this file must define this.
jinue_root 			?= .

# project directory structure
includes             = $(jinue_root)/include
lib                  = $(jinue_root)/lib
scripts              = $(jinue_root)/scripts

hal_includes         = $(includes)/hal
kstdc_includes         = $(includes)/kstdc
libjinue             = $(lib)/jinue

# object files
objects.c            = $(sources.c:%.c=%.o)
objects.nasm         = $(sources.nasm:%.asm=%.o)
objects.extra        =
objects              = $(objects.c) $(objects.nasm) $(objects.extra)

# built targets
targets.phony       ?=
targets             ?= $(target)
targets.all          = $(targets) $(targets.phony)

# files to clean up
unclean.build		 = $(objects) $(targets) $(objects:%.o=%.d) $(sources.nasm:%.asm=%.nasm)
unclean.extra        =
unclean				 = $(unclean.build) $(unclean.extra)
unclean_recursive   ?=

# debug/release build
#
# Anything other than DEBUG=yes means release
DEBUG   ?= yes

ifeq ($(DEBUG), yes)
	CPPFLAGS.debug		 =
	CFLAGS.debug 		 = -g3
	NASMFLAGS.debug		 =
else
	CPPFLAGS.debug		 = -DNDEBUG
	CFLAGS.debug 		 =
	NASMFLAGS.debug		 =
endif

# C preprocessor flags
#
# These flags are used when preprocessing C and assembly language files.
CPPFLAGS.includes    = -I$(includes) -I$(includes)/kstdc
CPPFLAGS.others      = -nostdinc
CPPFLAGS             = $(CPPFLAGS.includes) $(CPPFLAGS.debug) $(CPPFLAGS.others) $(CPPFLAGS.extra)

# C compiler flags
CFLAGS.warnings      = -std=c99 -pedantic -Wall -Werror=implicit-function-declaration -Werror=uninitialized
CFLAGS.arch          = -m32 -march=i686
CFLAGS.optimization  = -O3
CFLAGS.others        = -ffreestanding -fno-common -fno-omit-frame-pointer
CFLAGS               = $(CFLAGS.arch) $(CFLAGS.optimization) $(CFLAGS.debug) $(CFLAGS.warnings) $(CFLAGS.others) $(CFLAGS.extra)

# Linker flags
LDFLAGS.arch         = -Wl,-m,elf_i386
LDFLAGS.others       = -static -nostdlib
LDFLAGS              = $(LDFLAGS.arch) $(LDFLAGS.others) $(LDFLAGS.extra)

# NASM assembler flags
NASMFLAGS.arch       =
NASMFLAGS.others     =
NASMFLAGS            = $(NASMFLAGS.arch) $(NASMFLAGS.debug) $(NASMFLAGS.others) $(NASMFLAGS.extra)

# strip utility flags
STRIPFLAGS			= --strip-debug

# Automatic dependencies generation flags
DEPFLAGS			 = -MT $@ -MD -MP -MF $*.d

# Add dependencies generation flags when compiling object files from C source code
COMPILE.c = $(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c

# GRUB location (for creation of bootable ISO for virtual machine)
#
# This should probably be configurable through some sort of configure script
grub_modules         = /usr/lib/grub/i386-pc

# macro to include common target definitions (all, clean, implicit rules)
common               = $(jinue_root)/common.mk
