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
#include <kernel/infrastructure/i686/drivers/videofbfont.h>
#include <kernel/infrastructure/i686/pmap/pmap.h>
#include <kernel/infrastructure/i686/boot_alloc.h>
#include <kernel/infrastructure/i686/platform.h>
#include <kernel/interface/i686/bootinfo.h>
#include <kernel/machine/asm/machine.h>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>

/** width of a single character in pixels */
#define FONT_WIDTH  8

/** height of a single character in pixels */
#define FONT_HEIGHT 16

/** console abstraction */
static console_t console;

/** log ring buffer reader */
static log_reader_t log_reader;

/** colour configuration in RGB order, 8 bits per pixel */
const uint8_t reference_colours[][3] = {
    [JINUE_LOG_LEVEL_EMERGENCY] = {168, 0, 0},
    [JINUE_LOG_LEVEL_ALERT]     = {168, 0, 0},
    [JINUE_LOG_LEVEL_CRITICAL]  = {168, 0, 0},
    [JINUE_LOG_LEVEL_ERROR]     = {255, 87, 87},
    [JINUE_LOG_LEVEL_WARNING]   = {255, 255, 87},
    [JINUE_LOG_LEVEL_NOTICE]    = {87, 255, 87},
    [JINUE_LOG_LEVEL_INFO]      = {87, 255, 87},
    [JINUE_LOG_LEVEL_DEBUG]     = {87, 87, 87},
};

/** number of colours */
#define NUM_COLOURS (sizeof(reference_colours) / sizeof(reference_colours[0]))

/** colours transformed for the current pixel format */
uint8_t colours[NUM_COLOURS][3];

/** framebuffer/backbuffer configuration and state */
static struct {
    /* buffers */
    
    /** frame buffer */
    uint8_t     *framebuffer;
    /** back buffer */
    uint8_t     *backbuffer;

    /* configuration */

    /** buffer total size */
    size_t       size;
    /* total width in pixels */
    uint16_t     width;
    /** total height in pixels */
    uint16_t     height;
    /** total size of a line in bytes */
    uint16_t     pitch;
    /** start of viewport from the left in pixels */
    uint16_t     left;
    /** start of viewport from the top in pixels */
    uint16_t     top;
    /** pixel format - one of the VIDEO_PIXEL_FORMAT_xx constants */
    uint8_t      pixel_format;
    /** colour bytes per pixels */
    uint8_t      bpp;
    /** non-colour bytes per pixels */
    uint8_t      skip;

    /* state */

    /** Text line index of first text line in the back buffer
     * 
     * Used for zero copy scrolling. */
    uint16_t     origin;
} fb;

/** Initialize the configuration structure
 * 
 * @param bootinfo boot information structure
 */
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

    /* Varies according to pixel format but currently the same for all four
     * supported pixel formats. */
    fb.bpp = 3;
    fb.skip = bootinfo->video_depth / 8 - fb.bpp;
}

/** Initialize the colours array according to pixel format
 * 
 * Must be called after initialize_config().
 */
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

/** Erase the back buffer
 * 
 * To erase the frame buffer, erase the back buffer and then call
 * refresh_framebuffer().
 */
static void erase_backbuffer(void) {
    unsigned char *line_addr = fb.backbuffer;

    for(int y = 0; y < fb.height; ++y) {
        memset(line_addr, 0, fb.width * (fb.bpp + fb.skip));
        line_addr += fb.pitch;
    }

    fb.origin = 0;
}

/** Compute absolute row index from row index relative to origin */
static inline unsigned int from_origin(unsigned int row) {
    unsigned int sum = row + fb.origin;
    return sum < console.height ? sum : sum - console.height;
}

/** Compute byte offset of start of text row from row index */
static inline unsigned int row(unsigned int row) {
    return (row * FONT_HEIGHT + fb.top) * fb.pitch;
}

/** Compute byte offset from row start from column index */
static inline unsigned int column(unsigned int col) {
    return (col * FONT_WIDTH + fb.left) * (fb.bpp + fb.skip);
}

/** Copy pixels from the back buffer to the frane buffer */
static void refresh_framebuffer(void) {   
    /* When the origin is on line zero, we copy the whole back buffer content
     * instead of only the viewport lines.
     *
     * Edge case to consider: when we refresh after erasing the back buffer, we
     * want to copy all the lines so the whole frame buffer is also erased. */
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

/** Character write console callback function
 * 
 * @param c ASCII code of character to write
 * @param loglevel log level
 */
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

/** Console scrolling callback function
 * 
 * Scrolls up by one text line and erases the bottom line.
 */
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
 * @param boot_alloc boot-time memory allocator
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

    if(bootinfo->video_pitch < bootinfo->video_width * (bootinfo->video_depth / 8)) {
        warn(WARNING "disabling video framebuffer because information passed by bootloader is inconsistent (pitch).");
        return;
    }

    if(bootinfo->video_fb_size < bootinfo->video_height * bootinfo->video_pitch) {
        warn(WARNING "disabling video framebuffer because information passed by bootloader is inconsistent (size).");
        return;
    }

    bool has_pae = bootinfo_has_feature(bootinfo, BOOTINFO_FEATURE_PAE);

    if(bootinfo->video_fb_addr > ADDR_4GB && !has_pae) {
        warn(WARNING "disabling video framebuffer because it cannot be mapped without PAE.");
        return;
    }

    if(bootinfo->video_width > 1024 || bootinfo->video_height > 768) {
        /* Temporary limitation caused by memory management and performance
         * issues. Will be improved. */
        warn(WARNING "disabling video framebuffer because resolution is above 1024x768 supported maximum.");
        return;
    }

    if(bootinfo->video_fb_size > 10 * MB) {
        /* Temporary limitation caused by memory management and performance
         * issues. Will be improved. */
        warn(WARNING "disabling video framebuffer because it is larger than the supported 10MB.");
        return;
    }

    info(
        "Initializing video framebuffer for resolution %" PRIu16 "x%" PRIu16 ".",
        bootinfo->video_width,
        bootinfo->video_height
    );

    fb.backbuffer = boot_page_alloc_n(
        boot_alloc,
        (bootinfo->video_fb_size + PAGE_SIZE - 1) / PAGE_SIZE
    );

    fb.framebuffer = map_in_kernel(
        bootinfo->video_fb_addr,
        bootinfo->video_fb_size,
        JINUE_PROT_READ | JINUE_PROT_WRITE,
        JINUE_MAP_WRITE_COMBINE
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
