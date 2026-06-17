/*
 * Copyright (C) 2025-2026 Philippe Aubertin.
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

#include <kernel/domain/services/asm/cmdline.h>
#include <kernel/infrastructure/i686/asm/video.h>
#include <kernel/interface/i686/asm/boot.h>
#include <kernel/interface/i686/setup/asm/linux.h>
#include <kernel/interface/i686/setup/alloc.h>
#include <kernel/interface/i686/setup/linux.h>
#include <kernel/interface/i686/types.h>

#define HEADER_FIELD_PTR(h, t, o) (t)&(h)[o]

#define HEADER_FIELD(h, t, o) *(t*)&(h)[o]

/**
 * Copy the command line string
 * 
 * @param bootinfo boot information structure
 * @param linux_boot_params Linux x86 boot protocol real-mode kernel header
 */
static void copy_cmdline(bootinfo_t *bootinfo, const linux_boot_params_t linux_boot_params) {
    const char *src = HEADER_FIELD(linux_boot_params, char *, BOOT_CMD_LINE_PTR);

    size_t length = 0;

    if(src != NULL) {
        for(int idx = 0; idx < CMDLINE_MAX_PARSE_LENGTH && src[idx] != '\0'; ++idx) {
            ++length;
        }
    }

    char *dest          = alloc_pages(bootinfo, length + 1);
    bootinfo->cmdline   = dest;

    for(int idx = 0; idx < length; ++idx) {
        dest[idx] = src[idx];
    }

    dest[length] = '\0';
}

/**
 * Copy the ACPI (aka. e820) address map
 * 
 * @param bootinfo boot information structure
 * @param linux_boot_params Linux x86 boot protocol real-mode kernel header
 */
static void copy_acpi_address_map(bootinfo_t *bootinfo, const linux_boot_params_t linux_boot_params) {
    size_t size     = 20 * HEADER_FIELD(linux_boot_params, uint8_t, BOOT_ADDR_MAP_ENTRIES);
    const char *src = &linux_boot_params[BOOT_ADDR_MAP];
    char *dest      = alloc_pages(bootinfo, size);

    bootinfo->acpi_addr_map = (acpi_addr_range_t *)dest;

    for(int idx = 0; idx < size; ++idx) {
        dest[idx] = src[idx];
    }
}

/**
 * Identify the pixel format from Linux "screen info" video format information
 * 
 * Must be called with a screen information structure for a framebuffer video
 * format (EFI or VBE, not text).
 * 
 * @param screen_info Linux boot protocol "screen info" structure
 * @return pixel format ID, -1 if format is unsupported
 */
static int identify_pixel_format(const linux_screen_info_t *screen_info) {
    if(screen_info->red_size != 8 || screen_info->green_size != 8 || screen_info->blue_size != 8) {
        return -1;
    }

    if(screen_info->green_pos != 8) {
        return -1;
    }

    switch(screen_info->lfb_depth) {
    case 24:
        if(screen_info->red_pos == 0 && screen_info->blue_pos == 16) {
            return VIDEO_PIXEL_FORMAT_RGB888;
        }
        if(screen_info->red_pos == 16 && screen_info->blue_pos == 0) {
            return VIDEO_PIXEL_FORMAT_BGR888;
        }
    case 32:
        if(screen_info->red_pos == 0 && screen_info->blue_pos == 16) {
            return VIDEO_PIXEL_FORMAT_RGBA8888;
        }
        if(screen_info->red_pos == 16 && screen_info->blue_pos == 0) {
            return VIDEO_PIXEL_FORMAT_BGRA8888;
        }
    default:
        return -1;
    }

    return -1;
}

/** Set video information
 * 
 * This function maps information about the current video format passed by the
 * bootloader through the "screen information" structure.
 * 
 * @param bootinfo boot information structure
 * @param linux_boot_params Linux x86 boot protocol real-mode kernel header
 */
static void set_video_info(bootinfo_t *bootinfo, const linux_boot_params_t linux_boot_params) {
    const linux_screen_info_t *screen_info = (linux_screen_info_t *)&linux_boot_params[0];

    bootinfo->video_type = VIDEO_TYPE_NONE;

    switch(screen_info->orig_video_isVGA) {
    case LINUX_VIDEO_TYPE_VGA:
        if(screen_info->orig_video_mode == 3) {
            bootinfo->video_type = VIDEO_TYPE_TEXT;
        }
        return;
    case LINUX_VIDEO_TYPE_VLFB:
    case LINUX_VIDEO_TYPE_EFI:
        break;
    default:
        return;
    }

    int pixel_format = identify_pixel_format(screen_info);

    if(pixel_format < 0) {
        return;
    }

    bootinfo->video_type = VIDEO_TYPE_FRAMEBUFFER;
    bootinfo->video_depth = screen_info->lfb_depth;
    bootinfo->video_pixel_format = pixel_format;
    bootinfo->video_width = screen_info->lfb_width;
    bootinfo->video_height = screen_info->lfb_height;
    bootinfo->video_pitch = screen_info->lfb_linelength;
    
    bootinfo->video_fb_size = screen_info->lfb_size;
    bootinfo->video_fb_addr = screen_info->lfb_base;

    if(screen_info->orig_video_isVGA == LINUX_VIDEO_TYPE_VLFB) {
        bootinfo->video_fb_size <<= 16;
    }

    if(screen_info->capabilities & LINUX_VIDEO_CAPABILITY_64BIT_BASE) {
        bootinfo->video_fb_addr |= (uint64_t)screen_info->ext_lfb_base << 32;
    }
}

/**
 * Initialize some fields in the boot information structure from Linux header
 *
 * @param bootinfo boot information structure
 * @param linux_boot_params Linux x86 boot protocol real-mode kernel header
 */
void initialize_from_linux_boot_params(
    bootinfo_t                  *bootinfo,
    const linux_boot_params_t    linux_boot_params)
{
    bootinfo->ramdisk_start     = HEADER_FIELD(linux_boot_params, uint32_t, BOOT_RAMDISK_IMAGE);
    bootinfo->ramdisk_size      = HEADER_FIELD(linux_boot_params, uint32_t, BOOT_RAMDISK_SIZE);
    bootinfo->setup_signature   = HEADER_FIELD(linux_boot_params, uint32_t, BOOT_SETUP_HEADER);
    bootinfo->addr_map_entries  = HEADER_FIELD(linux_boot_params, uint8_t, BOOT_ADDR_MAP_ENTRIES);

    copy_acpi_address_map(bootinfo, linux_boot_params);

    copy_cmdline(bootinfo, linux_boot_params);

    set_video_info(bootinfo, linux_boot_params);
}
