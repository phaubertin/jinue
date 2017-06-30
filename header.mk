# relative path to the root of the project
#
# Makefiles that include this file should define this.
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
unclean              = $(objects) $(targets) $(sources.nasm:%.asm=%.nasm)

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
CFLAGS.others        = -ffreestanding -fno-common -fno-inline -fno-omit-frame-pointer
CFLAGS               = $(CFLAGS.arch) $(CFLAGS.optimization) $(CFLAGS.debug) $(CFLAGS.warnings) $(CFLAGS.others) $(CFLAGS.extra)

# Linker flags
LDFLAGS.arch         = -Wl,-m,elf_i386
LDFLAGS.others       = -static -nostdlib
LDFLAGS              = $(LDFLAGS.arch) $(LDFLAGS.others) $(LDFLAGS.extra)

# NASM assembler flags
NASMFLAGS.arch       = -felf
NASMFLAGS.others     =
NASMFLAGS            = $(NASMFLAGS.arch) $(NASMFLAGS.debug) $(NASMFLAGS.others) $(NASMFLAGS.extra)

# macro to include common target definitions (all, clean, implicit rules)
common               = $(jinue_root)/common.mk
