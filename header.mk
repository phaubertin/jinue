# relative path to the root of the project
#
# Makefiles that include this file should define this.
jinue_root 			?= .

# project directory structure
includes			 = $(jinue_root)/include
lib					 = $(jinue_root)/lib
scripts  			 = $(jinue_root)/scripts

hal_includes		 = $(includes)/hal
kstdc_includes		 = $(includes)/kstdc
libjinue			 = $(lib)/jinue

# debug/release build
#
# Anything other than DEBUG=yes means release
DEBUG   ?= yes

ifeq ($(DEBUG), yes)
	DEBUG_CPPFLAGS		 =
	DEBUG_CFLAGS 		 = -g3
	DEBUG_NASMFLAGS		 =
else
	DEBUG_CPPFLAGS		 = -DNDEBUG
	DEBUG_CFLAGS 		 =
	DEBUG_NASMFLAGS		 =
endif

# C preprocessor flags
#
# These flags are used when preprocessing C and assembly language files.
INCLUDES_CPPFLAGS	 = -I$(includes) -I$(includes)/kstdc
OTHER_CPPFLAGS		 = -nostdinc
CPPFLAGS			 = $(INCLUDES_CPPFLAGS) $(DEBUG_CPPFLAGS) $(OTHER_CPPFLAGS)

# C compiler flags
WARNING_CFLAGS		 = -std=c99 -pedantic -Wall -Werror=implicit-function-declaration -Werror=uninitialized
ARCH_CFLAGS			 = -m32 -march=i686
OPTIMIZATION_CFLAGS	 = -O3
OTHER_CFLAGS		 = -ffreestanding -fno-common -fno-inline -fno-omit-frame-pointer
CFLAGS				 = $(ARCH_CFLAGS) $(OPTIMIZATION_CFLAGS) $(DEBUG_CFLAGS) $(WARNING_CFLAGS) $(OTHER_CFLAGS)

# Linker flags
ARCH_LDFLAGS		 = -Wl,-m,elf_i386
OTHER_LDFLAGS		 = -static -nostdlib
LDFLAGS 			 = $(ARCH_LDFLAGS) $(OTHER_LDFLAGS)

# NASM assembler flags
ARCH_NASMFLAGS		 = -felf
OTHER_NASMFLAGS		 =
NASMFLAGS			 = $(ARCH_NASMFLAGS) $(DEBUG_NASMFLAGS) $(OTHER_NASMFLAGS)
