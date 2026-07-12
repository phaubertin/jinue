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
#include <kernel/infrastructure/i686/barriers.h>
#include <kernel/infrastructure/i686/platform.h>
#include <kernel/interface/i686/bootinfo.h>
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
uint8_t colours[NUM_COLOURS][4];

/** framebuffer configuration */
static struct {
    /** total width in pixels */
    unsigned int     width;
    /** total height in pixels */
    unsigned int     height;
    /** total size of a line in bytes */
    unsigned int     pitch;
    /** pixel format - one of the VIDEO_PIXEL_FORMAT_xx constants */
    unsigned int     pixel_format;
    /** frame buffer base address */
    uint8_t         *base_addr;
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
}

/** Initialize the colours array according to pixel format
 * 
 * Must be called after initialize_config().
 */
static void initialize_colours(void) {
    for(int idx = 0; idx < NUM_COLOURS; ++idx) {
        switch(fb.pixel_format) {
        case VIDEO_PIXEL_FORMAT_RGB888:
        case VIDEO_PIXEL_FORMAT_RGBA8888:
            colours[idx][0] = reference_colours[idx][0];
            colours[idx][1] = reference_colours[idx][1];
            colours[idx][2] = reference_colours[idx][2];
            /* Only relevant for RGBA 8888 but won't hurt otherwise since it
             * just won't be read. */
            colours[idx][3] = 0;
            break;
        case VIDEO_PIXEL_FORMAT_BGR888:
        case VIDEO_PIXEL_FORMAT_BGRA8888:
            colours[idx][0] = reference_colours[idx][2];
            colours[idx][1] = reference_colours[idx][1];
            colours[idx][2] = reference_colours[idx][0];
            colours[idx][3] = 0;
            break;
        }
    }
}

/** map pixel format to bytes per pixel
 * 
 * This function returns -1 if the pixel format is unsupported. This is checked
 * during initialization and does not need to be handled elsewhere.
 * 
 * @param pixel_format pixel format (VIDEO_PIXEL_FORMAT_xx value)
 * @return bytes per pixel, -1 for unsupported format
 */
static int map_bpp(unsigned int pixel_format) {
    switch(fb.pixel_format) {
    case VIDEO_PIXEL_FORMAT_RGB888:
    case VIDEO_PIXEL_FORMAT_BGR888:
        return 3;
    case VIDEO_PIXEL_FORMAT_RGBA8888:
    case VIDEO_PIXEL_FORMAT_BGRA8888:
        return 4;
    default:
        return -1;
    }
}

/** Erase the framebuffer */
static void clear_framebuffer(void) {
    unsigned char *line_addr = fb.base_addr;

    const unsigned int bpp = map_bpp(fb.pixel_format);

    for(int y = 0; y < fb.height; ++y) {
        memset(line_addr, 0, fb.width * bpp);
        line_addr += fb.pitch;
    }

    store_barrier();
}

// TODO make this function static again
/** Refresh the framebuffer with the content of the console text buffer */
void refresh_framebuffer(void) {
    const unsigned int viewport_height = console.height * FONT_HEIGHT;
    
    /* Let's center the viewport vertically. Let's not center horizontally to
     * preserve alignment. */
    const unsigned int top = (fb.height - viewport_height) / 2;
    const unsigned int bpp = map_bpp(fb.pixel_format);

    for(unsigned int y = 0; y < viewport_height; ++y) {
        uint8_t *wrptr = &fb.base_addr[(y + top) * fb.pitch];

        unsigned char *text_line = &console.buffer[2 * (y / FONT_HEIGHT) * console.width];

        for(unsigned int col = 0; col < console.width; ++col) {
            uint8_t c = text_line[2 * col] - 0x20;
            uint8_t colour_index = text_line[2 * col + 1];
            
            const uint8_t *const colour = colours[colour_index];
            const uint8_t font_byte = videofbfont[c * FONT_HEIGHT + y % FONT_HEIGHT];

            for(uint8_t mask = 0x80; mask != 0; mask >>= 1) {
                if(bpp == 4) {
                    *(uint32_t *)wrptr =  (font_byte & mask) ? *(uint32_t *)colour : 0;
                    wrptr += 4;
                }
                else {
                    for(unsigned int byte_index = 0; byte_index < bpp; ++byte_index) {
                        *(wrptr++) = (font_byte & mask) ? colour[byte_index] : 0;
                    }
                }
            }
        }
    }

    store_barrier();
}

/** Map a log level to the appropriate colour number.
 * 
 * @param loglevel the log level
 * @return colour number
 */
static uint8_t map_colour(int loglevel) {
    if(loglevel > JINUE_LOG_LEVEL_DEBUG) {
        return JINUE_LOG_LEVEL_DEBUG;
    }

    return loglevel;
}

/** Logging callback function.
 * 
 * This function is registered as the logging callback function and is called
 * by the logging infrastructure for each logging event.
 * 
 * @param event logging event
*/
static void do_log(const log_event_t *event) {
    console_write(
        &console,
        event->message,
        event->length,
        map_colour(event->loglevel)
    );

    refresh_framebuffer();
}

/** Initialize video framebuffer for logging.
 * 
 * @param config kernel configuration
 * @param bootinfo boot information structure
 * @param boot_alloc boot-time memory allocator
 */
void init_video_framebuffer(
    const config_t      *config,
    const bootinfo_t    *bootinfo,
    boot_alloc_t        *boot_alloc
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

    const int bpp = map_bpp(bootinfo->video_pixel_format);

    if(bpp < 0) {
        warn(WARNING "disabling video framebuffer because pixel format is unsupported.");
        return;
    }

    if(bootinfo->video_pitch < bootinfo->video_width * bpp) {
        warn(WARNING "disabling video framebuffer because information passed by bootloader is inconsistent (pitch).");
        return;
    }

    const size_t mapping_size = bootinfo->video_height * bootinfo->video_pitch;

    if(bootinfo->video_fb_size < mapping_size) {
        warn(WARNING "disabling video framebuffer because information passed by bootloader is inconsistent (size).");
        return;
    }

    bool has_pae = bootinfo_has_feature(bootinfo, BOOTINFO_FEATURE_PAE);

    if(bootinfo->video_fb_addr > ADDR_4GB && !has_pae) {
        warn(WARNING "disabling video framebuffer because it cannot be mapped without PAE.");
        return;
    }

    if(mapping_size > 128 * MB) {
        warn(WARNING "disabling video framebuffer because it is larger than the supported 128MB.");
        return;
    }

    info(
        "Initializing video framebuffer for resolution %" PRIu16 "x%" PRIu16 ".",
        bootinfo->video_width,
        bootinfo->video_height
    );

    initialize_config(bootinfo);

    initialize_colours();

    fb.base_addr = map_in_kernel(
        bootinfo->video_fb_addr,
        bootinfo->video_fb_size,
        JINUE_PROT_READ | JINUE_PROT_WRITE,
        JINUE_MAP_WRITE_COMBINE | JINUE_MAP_LARGE_PAGES
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
