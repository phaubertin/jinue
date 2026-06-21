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
    uint8_t     *framebuffer;
    uint8_t     *backbuffer;
    size_t       size;
    uint16_t     origin;
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
} fb;

static void initialize_config(const bootinfo_t *bootinfo) {
    fb.width        = bootinfo->video_width;
    fb.height       = bootinfo->video_height;
    fb.pitch        = bootinfo->video_pitch;
    fb.pixel_format = bootinfo->video_pixel_format;
    fb.size         = bootinfo->video_fb_size;
    fb.origin       = 0;

    const unsigned int viewport_width = console.width * FONT_WIDTH;
    const unsigned int viewport_height = console.height * FONT_HEIGHT;

    /* Center viewport/text area */
    fb.left             = (fb.width - viewport_width) / 2;
    fb.top              = (fb.height - viewport_height) / 2;
    fb.viewport_width   = viewport_width;
    fb.viewport_height  = viewport_height;

    /* Varies according to pixel format but currently the same for all four
     * supported pixel formats. */
    fb.bpp = 3;
    fb.skip = bootinfo->video_depth / 8 - fb.bpp;
}

static void initalize_colours(void) {
    for(int idx = 0; idx < NUM_COLOURS; ++idx) {
        switch(fb.pixel_format) {
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
    unsigned char *line_addr = fb.backbuffer;

    for(int y = 0; y < fb.height; ++y) {
        memset(line_addr, 0, fb.width * (fb.bpp + fb.skip));
        line_addr += fb.pitch;
    }
}

static inline unsigned int from_origin(unsigned int row) {
    unsigned int sum = row + fb.origin;
    return sum < console.height ? sum : sum - console.height;
}

static inline unsigned int row(unsigned int row) {
    return (row * FONT_HEIGHT + fb.top) * fb.pitch;
}

static inline unsigned int column(unsigned int col) {
    return (col * FONT_WIDTH + fb.left) * (fb.bpp + fb.skip);
}

static void refresh_framebuffer(void) {   
    /* When we refresh after erasing the back buffer, we want to copy all the
     * lines, not only the viewport lines. */
    if(fb.origin == 0) {
        memcpy(fb.framebuffer, fb.backbuffer, fb.size);
        return;
    }

    unsigned int top_rows = console.height - from_origin(0);

    memcpy(
        &fb.framebuffer[row(0)],
        &fb.backbuffer[row(from_origin(0))],
        top_rows * FONT_HEIGHT * fb.pitch
    );

    memcpy(
        &fb.framebuffer[row(top_rows)],
        &fb.backbuffer[row(0)],
        (console.height - top_rows) * FONT_HEIGHT * fb.pitch
    );
}

static void do_write(unsigned char c, uint8_t loglevel) {
    const uint8_t *const colour = colours[
        loglevel < JINUE_LOG_LEVEL_DEBUG ? loglevel : JINUE_LOG_LEVEL_DEBUG
    ];

    uint8_t *start = &fb.backbuffer[row(from_origin(console.row)) + column(console.col)];

    for(unsigned int y = 0; y < FONT_HEIGHT; ++y) {
        const uint8_t font_byte = videofbfont[(c - 0x20) * FONT_HEIGHT + y];
        
        uint8_t *wrptr = start;

        for(uint8_t mask = 0x80; mask != 0; mask >>= 1) {
            for(unsigned int byte_index = 0; byte_index < fb. bpp; ++byte_index) {
                *(wrptr++) = (font_byte & mask) ? colour[byte_index] : 0;
            }
            wrptr += fb.skip;
        }

        start += fb.pitch;
    }
}

static void do_scroll(void) {
    fb.origin = from_origin(1);

    /* Erase the bottom line */
    memset(&fb.backbuffer[row(from_origin(console.height - 1))], 0, FONT_HEIGHT * fb.pitch);
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

    if(bootinfo->video_width > 1024 || bootinfo->video_height > 768) {
        /* Temporary limitation caused by memory management and performance
         * issues. Will be improved. */
        warn(WARNING "disabling video framebuffer because resolution is above 1024x768 supported maximum.");
        return;
    }

    info(
        "Initializing video framebuffer for resolution %" PRIu16 "x%" PRIu16 ".",
        bootinfo->video_width,
        bootinfo->video_height
    );

    /* TODO we should do some validation before blindly trusting these values */
    /* TODO deal with the 4GB limit if PAE is disabled */

    fb.backbuffer = boot_page_alloc_n(
        boot_alloc,
        (bootinfo->video_fb_size + PAGE_SIZE - 1) / PAGE_SIZE
    );

    fb.framebuffer = map_in_kernel(
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
