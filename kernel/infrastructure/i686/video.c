/*
 * Copyright (C) 2026 Philippe Aubertin.
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

#include <kernel/domain/services/logging.h>
#include <kernel/infrastructure/i686/asm/video.h>
#include <kernel/infrastructure/i686/platform.h>
#include <kernel/infrastructure/i686/video.h>
#include <inttypes.h>

/** Map the video type to a human-readable string representation
 * 
 * @param video_type video type ID
 * @return human-readable string
 */
static const char *video_type_string(int video_type) {
    switch(video_type) {
    case VIDEO_TYPE_NONE:
        return "none";
    case VIDEO_TYPE_TEXT:
        return "text";
    case VIDEO_TYPE_FRAMEBUFFER:
        return "framebuffer";
    default:
        return "???";
    }
}

/** Map the pixel format to a human-readable string representation
 * 
 * @param pixel_format pixel format ID
 * @return human-readable string
 */
static const char *pixel_format_string(int pixel_format) {
    switch(pixel_format) {
    case VIDEO_PIXEL_FORMAT_RGB888:
        return "RGB888";
    case VIDEO_PIXEL_FORMAT_BGR888:
        return "BGR888";
    case VIDEO_PIXEL_FORMAT_RGBA8888:
        return "RGBA8888";
    case VIDEO_PIXEL_FORMAT_BGRA8888:
        return "BGRA8888";
    default:
        return "???";
    }
}

/**
 * Log video format information
 * 
 * @param bootinfo Boot information structure
 */
void dump_video_information(const bootinfo_t *bootinfo) {
    int video_type = platform_get_video_type();

    info("Video information:");
    info("  type: %s", video_type_string(video_type));

    if(video_type == VIDEO_TYPE_FRAMEBUFFER) {
        info("  resolution: %" PRIu16 "x%" PRIu16,  bootinfo->video_width, bootinfo->video_height);
        info("  depth: %" PRIu8, bootinfo->video_depth);
        info("  pixel format: %s", pixel_format_string(bootinfo->video_pixel_format));
        info("  pitch: %" PRIu16, bootinfo->video_pitch);
        info("  framebuffer: addr 0x%" PRIx64 " size 0x%" PRIx32, bootinfo->video_fb_addr, bootinfo->video_fb_size);
    }
}
