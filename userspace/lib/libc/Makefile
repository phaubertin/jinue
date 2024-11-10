# Copyright (C) 2022 Philippe Aubertin.
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

jinue_root = ../../..
include $(jinue_root)/header.mk

sources.c        = \
	i686/stack.c \
	pthread/attr.c \
	pthread/libc.c \
	pthread/thread.c \
	assert-kernel.c \
	brk.c \
	descriptors.c \
	errno.c \
	getauxval.c \
	getenv.c \
	init.c \
	malloc.c \
	mmap.c \
	physmem.c \
	snprintf.c \
	string.c \
	vsnprintf.c
sources.nasm     = i686/crt.asm i686/pthread.asm

target.user      = libc.a
target.pthread	 = libpthread.a
target.kernel    = libc-kernel.a
targets          = $(target.user) $(target.pthread) $(target.kernel)

objects.generic  = string.o snprintf.o vsnprintf.o
objects.user     = \
	i686/crt.o \
	pthread/libc.o \
	brk.o \
	descriptors.o \
	errno.o \
	getauxval.o \
	getenv.o init.o \
	malloc.o \
	mmap.o \
	physmem.o
objects.pthread  = \
	i686/pthread.o \
	i686/stack.o \
	pthread/attr.o \
	pthread/thread.o
objects.kernel   = assert-kernel.o

include $(common)

$(target.user): $(objects.generic) $(objects.user)

$(target.pthread): $(objects.pthread)

$(target.kernel): $(objects.generic) $(objects.kernel)

%.a:
	ar rcs $@ $^
