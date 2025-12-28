/*
 * Copyright (C) 2022-2025 Philippe Aubertin.
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
#include <kernel/domain/services/logging.h>
#include <kernel/machine/spinlock.h>
#include <kernel/utils/list.h>
#include <kernel/utils/utils.h>
#include <kernel/types.h>
#include <stdbool.h>
#include <stdio.h>

#define RING_BUFFER_SIZE    65536

#define EVENT_ALIGNMENT     4

static char ring_buffer[RING_BUFFER_SIZE];

#define RING_BUFFER_END (ring_buffer + RING_BUFFER_SIZE)

/** log consumers */
static list_t loggers = LIST_INITIALIZER;

static spinlock_t logging_spinlock = SPINLOCK_INITIALIZER;

static struct {
    const char  *start_ptr;
    uint64_t     start_id;
    char        *write_ptr;
    uint64_t     write_id;
    
} state = {
    .start_ptr  = ring_buffer,
    .start_id   = 0,
    .write_ptr  = ring_buffer,
    .write_id   = 0
};

/**
 * Set up log consumer to read from the start of the ring buffer
 * 
 * This is done:
 *  1) When a new consumer is registered.
 *  2) When a consumer is so far behind that it's next log event is no longer
 *     in the ring buffer.
 * 
 * @param logger log consumer
 */
static void initialize_at_start(logger_t *logger) {
    logger->read_ptr    = state.start_ptr;
    logger->read_id     = state.start_id;
}

/**
 * Register a log consumer
 * 
 * @param logger log consumer
 */
void register_logger(logger_t *logger) {
    spin_lock(&logging_spinlock);

    initialize_at_start(logger);
    list_enqueue(&loggers, &logger->loggers);

    spin_unlock(&logging_spinlock);
}

/** offset to the next log event */
#define NEXT(event) (sizeof(log_event_t) + ALIGN_END(event->length, EVENT_ALIGNMENT))

static void start_go_next(void) {
    const log_event_t *start = (const log_event_t *)state.start_ptr;
    state.start_ptr += NEXT(start);

    ++state.start_id;
}

static void write_go_next(void) {
    const log_event_t *event = (const log_event_t *)state.write_ptr;
    state.write_ptr += NEXT(event);

    ++state.write_id;
}

static void read_go_next(logger_t *logger) {
    const log_event_t *event = (log_event_t *)logger->read_ptr;
    logger->read_ptr += NEXT(event);

    ++logger->read_id;
}

static void reset_start(void) {
    state.start_ptr = ring_buffer;
}

static void reset_write(void) {
    state.write_ptr = ring_buffer;
}

static void reset_read(logger_t *logger) {
    logger->read_ptr = ring_buffer;
}

static void write_terminator(void) {
    log_event_t *event = (log_event_t *)state.write_ptr;
    event->length = 0;
}

static bool start_is_at_terminator(void) {
    const log_event_t *event = (const log_event_t *)state.start_ptr;
    return event->length == 0;
}

static bool read_is_at_terminator(logger_t *logger) {
    const log_event_t *event = (log_event_t *)logger->read_ptr;
    return event->length == 0;
}

static bool start_is_in_redzone(void) {
    /* Space reserved for the next log event. We don't yet know the length of
     * the log message, so we assume worst case (JINUE_PUTS_MAX_LENGTH). We
     * want to be able to write the log event, with its message and header, and
     * still have enough space to be able to add a terminating header if needed. */
#define REDZONE_SIZE (JINUE_PUTS_MAX_LENGTH + 2 * sizeof(log_event_t))

    const char *redzone_end = state.write_ptr + REDZONE_SIZE;

    return state.start_ptr >= state.write_ptr && state.start_ptr < redzone_end;
}

/**
 * Catch up a log consumer to all events in the ring buffer
 * 
 * @param logger log consumer
 */
static void sync(logger_t *logger) {
    if(logger->read_id < state.start_id) {
        initialize_at_start(logger);
    }

    while(logger->read_id < state.write_id) {
        if(read_is_at_terminator(logger)) {
            /* We reached the end of the buffer. Let's continue reading from
             * the start. */
            reset_read(logger);
        }

        const log_event_t *event = (log_event_t *)logger->read_ptr;

        read_go_next(logger);
        
        logger->log(event);
    }
}

/**
 * Catch up all log consumers to all events in the ring buffer
 */
static void sync_all(void) {
    list_cursor_t cur = list_head(&loggers);

    while(true) {
        logger_t *logger = list_cursor_entry(cur, logger_t, loggers);

        if(logger == NULL) {
            break;
        }

        sync(logger);        

        cur = list_cursor_next(cur);
    }
}

/**
 * Log a message
 * 
 * This is the common code for error(), warning() and info().
 * 
 * @param loglevel log message level/priority
 * @param source source of log: LOG_SOURCE_KERNEL or LOG_SOURCE_USERSPACE
 * @param format printf-style format string for the message
 * @param args format string arguments
 */
static void log_message(int loglevel, int source, const char *restrict format, va_list args) {
    spin_lock(&logging_spinlock);



    if(state.write_ptr + REDZONE_SIZE > RING_BUFFER_END) {
        /* Not enough space is left at the end of the buffer, so we add a
         * terminator and continue writing from the start of the ring buffer.
         *
         * Before we do this, we need to get the start pointer out of the
         * way by scanning for the existing terminator, updating start_id as
         * we do it. */
        while(start_is_in_redzone()) {
            start_go_next();

            if(start_is_at_terminator()) {
                reset_start();
            }
        }

        /* With the start pointer out of the way, we add the terminator and
         * set the write pointer to the start of the ring buffer. */
        write_terminator();
        reset_write();
        
        state.write_ptr = ring_buffer;
    }

    /* Here, the write pointer is where we want it to be. Before we can write
     * the new event, we need to push the start pointer if it is in the red
     * zone.
     * 
     * Edge case to consider: when writing the very first event, the start
     * pointer is at the beginning of the ring buffer and we want it to stay
     * there. */
    if(state.write_id != 0) {
        while(start_is_in_redzone()) {
            start_go_next();

            if(start_is_at_terminator()) {
                reset_start();
            }
        }
    }

    log_event_t *event  = (log_event_t *)state.write_ptr;
    event->loglevel     = loglevel;
    event->source       = source;
    event->length       = vsnprintf(event->message, JINUE_PUTS_MAX_LENGTH + 1, format, args);

    write_go_next();

    sync_all();

    spin_unlock(&logging_spinlock);
}

/**
 * Log a message
 * 
 * @param loglevel log message level/priority
 * @param source source of log: LOG_SOURCE_KERNEL or LOG_SOURCE_USERSPACE
 * @param format printf-style format string for the message
 * @param ... format string arguments
 */
void log(int loglevel, int source, const char *restrict format, ...) {
    va_list args;

    va_start(args, format);

    log_message(loglevel, source, format, args);

    va_end(args);
}

/**
 * Log a general information message from the kernel
 * 
 * @param format printf-style format string for the message
 * @param ... format string arguments
 */
void info(const char *restrict format, ...) {
    va_list args;

    va_start(args, format);

    log_message(JINUE_PUTS_LOGLEVEL_INFO, LOG_SOURCE_KERNEL, format, args);

    va_end(args);
}

/**
 * Log a warning message from the kernel
 * 
 * @param format printf-style format string for the message
 * @param ... format string arguments
 */
void warn(const char *restrict format, ...) {
    va_list args;

    va_start(args, format);

    log_message(JINUE_PUTS_LOGLEVEL_WARNING, LOG_SOURCE_KERNEL, format, args);

    va_end(args);
}

/**
 * Log an error message from the kernel
 * 
 * @param format printf-style format string for the message
 * @param ... format string arguments
 */
void error(const char *restrict format, ...) {
    va_list args;

    va_start(args, format);

    log_message(JINUE_PUTS_LOGLEVEL_ERROR, LOG_SOURCE_KERNEL, format, args);

    va_end(args);
}
