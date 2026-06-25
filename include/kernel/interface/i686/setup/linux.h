/*
 * Copyright (C) 2025 Philippe Aubertin.
 * All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the author nor the names of other contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef JINUE_KERNEL_INTERFACE_I686_SETUP_LINUX_H
#define JINUE_KERNEL_INTERFACE_I686_SETUP_LINUX_H

#include <kernel/infrastructure/compiler.h>
#include <kernel/interface/i686/types.h>

typedef char linux_boot_params_t[];

typedef PACKED_STRUCT {
	uint8_t     orig_x;
	uint8_t     orig_y;
	uint16_t    ext_mem_k;
	uint16_t    orig_video_page;
	uint8_t     orig_video_mode;
	uint8_t     orig_video_cols;
	uint8_t     flags;
	uint8_t     unused2;
	uint16_t    orig_video_ega_bx;
	uint16_t    unused3;
	uint8_t     orig_video_lines;
	uint8_t     orig_video_isVGA;
	uint16_t    orig_video_points;
	uint16_t    lfb_width;
	uint16_t    lfb_height;
	uint16_t    lfb_depth;
	uint32_t    lfb_base;
	uint32_t    lfb_size;
	uint16_t    cl_magic;
    uint16_t    cl_offset;
	uint16_t    lfb_linelength;
	uint8_t     red_size;
	uint8_t     red_pos;
	uint8_t     green_size;
	uint8_t     green_pos;
	uint8_t     blue_size;
	uint8_t     blue_pos;
	uint8_t     rsvd_size;
	uint8_t     rsvd_pos;
	uint16_t    vesapm_seg;
	uint16_t    vesapm_off;
	uint16_t    pages;
	uint16_t    vesa_attributes;
	uint32_t    capabilities;
	uint32_t    ext_lfb_base;
	uint8_t     reserved[2];
} linux_screen_info_t;

void initialize_from_linux_boot_params(
    bootinfo_t                  *bootinfo,
    const linux_boot_params_t    linux_boot_params);

#endif
