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
#include <kernel/infrastructure/i686/boot_alloc.h>
#include <kernel/infrastructure/i686/platform.h>
#include <kernel/machine/asm/machine.h>
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
    uint16_t     viewport_width;
    uint16_t     viewport_height;
    uint16_t     left;
    uint16_t     top;
    uint8_t      pixel_format;
    uint8_t      bpp;
    uint8_t      skip;
    uint8_t     *framebuffer;
    uint8_t     *backbuffer;
    size_t       size;
} fbconfig;

static void initialize_config(const bootinfo_t *bootinfo) {
    fbconfig.width          = bootinfo->video_width;
    fbconfig.height         = bootinfo->video_height;
    fbconfig.pitch          = bootinfo->video_pitch;
    fbconfig.pixel_format   = bootinfo->video_pixel_format;
    fbconfig.size           = bootinfo->video_fb_size;

    const unsigned int viewport_width = console.width * FONT_WIDTH;
    const unsigned int viewport_height = console.height * FONT_HEIGHT;

    /* Center viewport/text area */
    fbconfig.left               = (fbconfig.width - viewport_width) / 2;
    fbconfig.top                = (fbconfig.height - viewport_height) / 2;
    fbconfig.viewport_width     = viewport_width;
    fbconfig.viewport_height    = viewport_height;

    /* Varies according to pixel format but currently the same for all four
     * supported pixel formats. */
    fbconfig.bpp = 3;
    fbconfig.skip = bootinfo->video_depth / 8 - fbconfig.bpp;
}

static void initalize_colours(void) {
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

static void erase_backbuffer(void) {
    unsigned char *line_addr = fbconfig.backbuffer;

    for(int y = 0; y < fbconfig.height; ++y) {
        memset(line_addr, 0, fbconfig.width * (fbconfig.bpp + fbconfig.skip));
        line_addr += fbconfig.pitch;
    }
}

static void refresh_framebuffer(void) {
    memcpy(fbconfig.framebuffer, fbconfig.backbuffer, fbconfig.size);
}

static void do_write(unsigned char c, uint8_t loglevel) {
    const uint8_t *const colour = colours[
        loglevel < JINUE_LOG_LEVEL_DEBUG ? loglevel : JINUE_LOG_LEVEL_DEBUG
    ];

    const unsigned int row = console.row * FONT_HEIGHT + fbconfig.top;
    const unsigned int col = console.col * FONT_WIDTH + fbconfig.left;

    uint8_t *start = &fbconfig.backbuffer[row * fbconfig.pitch + col * (fbconfig.bpp + fbconfig.skip)];

    for(unsigned int y = 0; y < FONT_HEIGHT; ++y) {
        const uint8_t font_byte = videofbfont[(c - 0x20) * FONT_HEIGHT + y];
        
        uint8_t *wrptr = start;

        for(uint8_t mask = 0x80; mask != 0; mask >>= 1) {
            for(unsigned int byte_index = 0; byte_index <fbconfig. bpp; ++byte_index) {
                *(wrptr++) = (font_byte & mask) ? colour[byte_index] : 0;
            }
            wrptr += fbconfig.skip;
        }

        start += fbconfig.pitch;
    }
}

static void do_scroll(void) {
    uint8_t *wrptr = &fbconfig.backbuffer[fbconfig.top * fbconfig.pitch];
    const uint8_t *rdptr = wrptr + FONT_HEIGHT * fbconfig.pitch;

    const unsigned int count = (console.height - 1) * FONT_HEIGHT * fbconfig.pitch;

    for(unsigned int idx = 0; idx < count; ++idx) {
        *(wrptr++) = *(rdptr++);
    }

    /* Erase the bottom line */
    const unsigned int row = (console.height - 1) * FONT_HEIGHT + fbconfig.top;
    memset(&fbconfig.backbuffer[row * fbconfig.pitch], 0, FONT_HEIGHT * fbconfig.pitch);
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

    /* TODO we should do some validation before blindly trusting these values */
    /* TODO deal with the 4GB limit if PAE is disabled */

    fbconfig.backbuffer = boot_page_alloc_n(
        boot_alloc,
        (bootinfo->video_fb_size + PAGE_SIZE - 1) / PAGE_SIZE
    );

    fbconfig.framebuffer = map_in_kernel(
        bootinfo->video_fb_addr,
        bootinfo->video_fb_size,
        JINUE_PROT_READ | JINUE_PROT_WRITE
    ); 

    initialize_console(
        &console,
        bootinfo->video_width / FONT_WIDTH,
        bootinfo->video_height / FONT_HEIGHT,
        do_write,
        do_scroll
    );

    initialize_config(bootinfo);

    initalize_colours();   

    erase_backbuffer();

    refresh_framebuffer();

    initialize_log_reader(&log_reader, do_log);

    register_log_reader(&log_reader);
}
