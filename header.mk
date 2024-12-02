# Copyright (C) 2019-2024 Philippe Aubertin.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 
# 3. Neither the name of the author nor the names of other contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# ------------------------------------------------------------------------------
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
jinue_root          ?= .

# project directory structure
devel                = $(jinue_root)/devel
includes             = $(jinue_root)/include
kernel               = $(jinue_root)/kernel
kernel_img           = $(kernel)/interface/i686/jinue
scripts              = $(jinue_root)/scripts
userspace            = $(jinue_root)/userspace
qemu                 = $(devel)/qemu
virtualbox           = $(devel)/virtualbox
lib                  = $(userspace)/lib
libjinue             = $(lib)/jinue
libc                 = $(lib)/libc
zlib                 = $(lib)/zlib
bzip2                = $(lib)/bzip2
loader               = $(userspace)/loader
testapp              = $(userspace)/testapp
testapp_initrd       = $(testapp)/jinue-testapp-initrd.tar.gz
libjinue_syscalls    = $(libjinue)/libjinue.a
libjinue_utils       = $(libjinue)/libjinue-utils.a

# object files
objects.c            = $(sources.c:%.c=%.o)
objects.nasm         = $(sources.nasm:%.asm=%-nasm.o)
objects.extra        =
objects              = $(objects.c) $(objects.nasm) $(objects.extra)

# Dependency files
depfiles.c           = $(sources.c:%.c=%.d)
depfiles.nasm        = $(sources.nasm:%.asm=%-nasm.d)
depfiles.extra       =
depfiles             = $(depfiles.c) $(depfiles.nasm) $(depfiles.extra)

# built targets
targets.phony       ?=
targets             ?= $(target)
targets.all          = $(targets) $(targets.phony)

# files to clean up
unclean.build        = $(objects) $(targets) $(depfiles) $(sources.nasm:%.asm=%.nasm)
unclean.extra        =
unclean              = $(unclean.build) $(unclean.extra)
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
CPPFLAGS.includes    = -I$(includes)
CPPFLAGS.arch        = -m32 -march=i686
CPPFLAGS.others      = -nostdinc
CPPFLAGS             = $(CPPFLAGS.arch) $(CPPFLAGS.includes) $(CPPFLAGS.debug) $(CPPFLAGS.others) $(CPPFLAGS.extra)

# C compiler flags
CFLAGS.warnings      = -std=c99 -pedantic -Wall -Wno-array-bounds -Werror=implicit -Werror=uninitialized -Werror=return-type
CFLAGS.arch          = -m32 -march=i686
CFLAGS.optimization  = -O3
CFLAGS.others        = -ffreestanding -fno-pie -fno-common -fno-omit-frame-pointer -fno-delete-null-pointer-checks
CFLAGS               = $(CFLAGS.arch) $(CFLAGS.optimization) $(CFLAGS.debug) $(CFLAGS.warnings) $(CFLAGS.others) $(CFLAGS.extra)

# Linker flags
LDFLAGS.arch         = -Wl,-m,elf_i386
LDFLAGS.others       = -static -nostdlib -Wl,-z,noexecstack
LDFLAGS              = $(LDFLAGS.arch) $(LDFLAGS.others) $(LDFLAGS.extra)

# NASM assembler flags
NASMFLAGS.arch       =
NASMFLAGS.others     =
NASMFLAGS            = $(NASMFLAGS.arch) $(NASMFLAGS.debug) $(NASMFLAGS.others) $(NASMFLAGS.extra)

# strip utility flags
STRIPFLAGS           = --strip-debug

# Automatic dependencies generation flags
DEPFLAGS             = -MT $@ -MD -MP -MF

# Add dependencies generation flags when compiling object files from C source code
COMPILE.c = $(CC) $(DEPFLAGS) $(basename $@).d $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c

# macro to include common target definitions (all, clean, implicit rules)
common               = $(jinue_root)/common.mk
