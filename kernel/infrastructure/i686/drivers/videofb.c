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

#include <jinue/shared/asm/logging.h>
#include <jinue/shared/asm/mman.h>
#include <kernel/domain/services/logging.h>
#include <kernel/domain/services/mman.h>
#include <kernel/infrastructure/i686/asm/video.h>
#include <kernel/infrastructure/i686/drivers/console.h>
#include <kernel/infrastructure/i686/drivers/videofb.h>
#include <kernel/infrastructure/i686/platform.h>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>

#define FONT_WIDTH  8

#define FONT_HEIGHT 16

extern const uint8_t videofbfont[];

/** console abstraction */
static console_t console;

/** log ring buffer reader */
static log_reader_t log_reader;

const uint8_t reference_colours[][3] = {
    /* RGB order, 8 bits per pixel */
    [JINUE_LOG_LEVEL_EMERGENCY] = {168, 0, 0},
    [JINUE_LOG_LEVEL_ALERT]     = {168, 0, 0},
    [JINUE_LOG_LEVEL_CRITICAL]  = {168, 0, 0},
    [JINUE_LOG_LEVEL_ERROR]     = {255, 87, 87},
    [JINUE_LOG_LEVEL_WARNING]   = {255, 255, 87},
    [JINUE_LOG_LEVEL_NOTICE]    = {87, 255, 87},
    [JINUE_LOG_LEVEL_INFO]      = {87, 255, 87},
    [JINUE_LOG_LEVEL_DEBUG]     = {87, 87, 87},
};

#define NUM_COLOURS (sizeof(reference_colours) / sizeof(reference_colours[0]))

uint8_t colours[NUM_COLOURS][3];

static struct {
    uint16_t     width;
    uint16_t     height;
    uint16_t     pitch;
    uint8_t      pixel_format;
    uint8_t      pixel_bytes;
    uint8_t     *base_addr;
} fbconfig;

static void initialize_config(const bootinfo_t *bootinfo) {
    fbconfig.width          = bootinfo->video_width;
    fbconfig.height         = bootinfo->video_height;
    fbconfig.pitch          = bootinfo->video_pitch;
    fbconfig.pixel_format   = bootinfo->video_pixel_format;
    fbconfig.pixel_bytes    = bootinfo->video_depth / 8;
}

static void iniitalize_colours(void) {
    for(int idx = 0; idx < NUM_COLOURS; ++idx) {
        switch(fbconfig.pixel_format) {
        case VIDEO_PIXEL_FORMAT_RGB888:
        case VIDEO_PIXEL_FORMAT_RGBA8888:
            colours[idx][0] = reference_colours[idx][0];
            colours[idx][1] = reference_colours[idx][1];
            colours[idx][2] = reference_colours[idx][2];
            break;
        case VIDEO_PIXEL_FORMAT_BGR888:
        case VIDEO_PIXEL_FORMAT_BGRA8888:
            colours[idx][0] = reference_colours[idx][2];
            colours[idx][1] = reference_colours[idx][1];
            colours[idx][2] = reference_colours[idx][0];
            break;
        }
    }
}

static void clear_framebuffer(void) {
    unsigned char *line_addr = fbconfig.base_addr;

    for(int y = 0; y < fbconfig.height; ++y) {
        memset(line_addr, 0, fbconfig.width * fbconfig.pixel_bytes);
        line_addr += fbconfig.pitch;
    }
}

static void refresh_framebuffer(void) {
    const unsigned int viewport_width = console.width * FONT_WIDTH;
    const unsigned int viewport_height = console.height * FONT_HEIGHT;
    
    /* Center viewport/text area */
    const unsigned int left = (fbconfig.width - viewport_width) / 2;
    const unsigned int top = (fbconfig.height - viewport_height) / 2;

    unsigned int bpp;
    unsigned int skip;

    switch(fbconfig.pixel_format) {
    case VIDEO_PIXEL_FORMAT_RGB888:
    case VIDEO_PIXEL_FORMAT_BGR888:
        bpp = 3;
        skip = 0;
        break;
    case VIDEO_PIXEL_FORMAT_RGBA8888:
    case VIDEO_PIXEL_FORMAT_BGRA8888:
        bpp = 3;
        skip = 1;
        break;
    default:
        return;
    }

    for(unsigned int y = 0; y < viewport_height; ++y) {
        uint8_t *wrptr = &fbconfig.base_addr[(y + top) * fbconfig.pitch + left * (bpp + skip)];

        unsigned char *text_line = &console.buffer[2 * (y / FONT_HEIGHT) * console.width];

        for(unsigned int col = 0; col < console.width; ++col) {
            uint8_t c = text_line[2 * col] - 0x20;
            uint8_t colour_index = text_line[2 * col + 1];

            if(colour_index > JINUE_LOG_LEVEL_DEBUG) {
                colour_index = JINUE_LOG_LEVEL_DEBUG;
            }

            const uint8_t *const colour = colours[colour_index];
            const uint8_t font_byte = videofbfont[c * FONT_HEIGHT + y % FONT_HEIGHT];

            for(uint8_t mask = 0x80; mask != 0; mask >>= 1) {
                for(unsigned int byte_index = 0; byte_index < bpp; ++byte_index) {
                    *(wrptr++) = (font_byte & mask) ? colour[byte_index] : 0;
                }
                wrptr += skip;
            }
        }
    }
}

/** Logging callback function.
 * 
 * This function is registered as the logging callback function and is called
 * by the logging infrastructure for each logging event.
 * 
 * @param event logging event
*/
static void do_log(const log_event_t *event) {
    console_write(&console, event->message, event->length, event->loglevel);

    refresh_framebuffer();
}

/** Initialize video framebuffer for logging.
 * 
 * @param config kernel configuration
 * @param bootinfo boot information structure
 */
void init_video_framebuffer(
    const config_t *config,
    const bootinfo_t *bootinfo,
    boot_alloc_t *boot_alloc
) {
    if(! config->machine.vga_enable) {
        return;
    }

    if(!platform_is_vga_present()) {
        return;
    }

    if(platform_get_video_type() != VIDEO_TYPE_FRAMEBUFFER) {
        return;
    }

    info(
        "Initializing video framebuffer for resolution %" PRIu16 "x%" PRIu16 ".",
        bootinfo->video_width,
        bootinfo->video_height
    );

    initialize_config(bootinfo);

    iniitalize_colours();

    /* TODO we should do some validation before blindly trusting these values */
    /* TODO deal with the 4GB limit if PAE is disabled */
    fbconfig.base_addr = map_in_kernel(
        bootinfo->video_fb_addr,
        bootinfo->video_fb_size,
        JINUE_PROT_READ | JINUE_PROT_WRITE
    );    

    clear_framebuffer();

    allocate_console(
        &console,
        boot_alloc,
        bootinfo->video_width / FONT_WIDTH,
        bootinfo->video_height / FONT_HEIGHT,
        JINUE_LOG_LEVEL_INFO
    );

    erase_console(&console);

    initialize_log_reader(&log_reader, do_log);

    register_log_reader(&log_reader);
}
