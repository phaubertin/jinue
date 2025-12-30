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

/** size of the log ring buffer */
#define RING_BUFFER_SIZE    65536

/** alignment of log events in the ring buffer */
#define EVENT_ALIGNMENT     4

/** static storage for the ring buffer */
static char ring_buffer[RING_BUFFER_SIZE];

/** end of the ring buffer */
#define RING_BUFFER_END (ring_buffer + RING_BUFFER_SIZE)

/** log readers */
static list_t readers = LIST_INITIALIZER;

/** lock to protect access to the ring buffer and its state */
static spinlock_t logging_spinlock = SPINLOCK_INITIALIZER;

/** state for the producer side of the log ring buffer */
static struct {
    char        *write_ptr;
    const char  *tail_ptr;
    uint64_t     write_id;
    uint64_t     tail_id;
} state = {
    .write_ptr  = ring_buffer,
    .tail_ptr   = ring_buffer,
    .write_id   = 0,
    .tail_id    = 0
};

/**
 * Set up a log reader to read from the start of the ring buffer
 * 
 * This is done:
 *  1) When a log reader is registered.
 *  2) When a reader is so far behind that it's next log event is no longer
 *     in the ring buffer.
 * 
 * @param reader log reader
 */
static void initialize_at_start(log_reader_t *reader) {
    reader->read_ptr    = state.tail_ptr;
    reader->read_id     = state.tail_id;
}

/**
 * Initialize a log reader
 * 
 * @param reader log reader
 * @param log logging function for the reader
 */
void initialize_log_reader(log_reader_t *reader, log_reader_func_t log) {
    reader->log = log;
    initialize_at_start(reader);
}

/**
 * Register a log reader
 * 
 * @param reader log reader
 */
void register_log_reader(log_reader_t *reader) {
    spin_lock(&logging_spinlock);

    list_enqueue(&readers, &reader->readers);

    spin_unlock(&logging_spinlock);
}

/**
 * Update the ring buffer state write pointer and ID to the next event
 */
static void write_go_next(void) {
#define NEXT(event) (sizeof(log_event_t) + ALIGN_END(event->length, EVENT_ALIGNMENT))

    const log_event_t *event = (const log_event_t *)state.write_ptr;
    state.write_ptr += NEXT(event);

    ++state.write_id;
}

/**
 * Update the ring buffer state tail pointer and ID to the next event
 */
static void tail_go_next(void) {
    const log_event_t *event = (const log_event_t *)state.tail_ptr;
    state.tail_ptr += NEXT(event);

    ++state.tail_id;
}

/**
 * Update the read pointer and ID of a log reader to the next event
 * 
 * @param reader log reader
 */
static void read_go_next(log_reader_t *reader) {
    const log_event_t *event = (log_event_t *)reader->read_ptr;
    reader->read_ptr += NEXT(event);

    ++reader->read_id;
}

/**
 * Update the ring buffer state write pointer to the start of the buffer
 */
static void write_go_to_buffer_start(void) {
    state.write_ptr = ring_buffer;
}

/**
 * Update the ring buffer state tail pointer to the start of the buffer
 */
static void tail_go_to_buffer_start(void) {
    state.tail_ptr = ring_buffer;
}

/**
 * Update the read pointer of a log reader to the start of the buffer
 * 
 * @param reader log reader
 */
static void read_go_to_buffer_start(log_reader_t *reader) {
    reader->read_ptr = ring_buffer;
}

/**
 * Write a terminator at the current position of the write pointer
 */
static void write_terminator(void) {
    log_event_t *event = (log_event_t *)state.write_ptr;
    event->length = 0;
}

/**
 * Check whether the ring buffer state tail pointer is at a terminator
 * 
 * @return true if at terminator, false otherwise
 */
static bool tail_is_at_terminator(void) {
    const log_event_t *event = (const log_event_t *)state.tail_ptr;
    return event->length == 0;
}

/**
 * Check whether the read pointer of a log reader is at a terminator
 * 
 * @param reader log reader
 * @return true if at terminator, false otherwise
 */
static bool read_is_at_terminator(log_reader_t *reader) {
    const log_event_t *event = (log_event_t *)reader->read_ptr;
    return event->length == 0;
}

/**
 * Check whether the specified pointer is in the redzone
 * 
 * The redzone is the space right after the write pointer where data will
 * possibly be overwritten when the next log event is written.
 * 
 * @param ptr pointer to check
 * @return true if tail ponter is in the redzone, false otherwise
 */
static bool pointer_is_in_redzone(const char *ptr) {
    /* Space reserved for the next log event. We only know the length of a log
     * message once it has been written, so we have to assume the worst case
     * (JINUE_LOG_MAX_LENGTH). We want to be able to write the log event, with
     * its message and header, and still have enough space to be able to add a
     * terminator (i.e. zero-length header) if needed. */
#define REDZONE_SIZE (JINUE_LOG_MAX_LENGTH + 2 * sizeof(log_event_t))

    const char *redzone_end = state.write_ptr + REDZONE_SIZE;

    return ptr >= state.write_ptr && ptr < redzone_end;
}

/**
 * Push the ring buffer state tail pointer out of the redzone
 */
static void push_tail(void) {
    while(pointer_is_in_redzone(state.tail_ptr)) {
        tail_go_next();

        if(tail_is_at_terminator()) {
            tail_go_to_buffer_start();
        }
    }
}

/**
 * Catch up a log reader to all events in the ring buffer
 * 
 * @param reader log reader
 */
static void sync_reader(log_reader_t *reader) {
    if(reader->read_id < state.tail_id) {
        initialize_at_start(reader);
    }

    while(reader->read_id < state.write_id) {
        if(read_is_at_terminator(reader)) {
            read_go_to_buffer_start(reader);
        }

        const log_event_t *event = (log_event_t *)reader->read_ptr;

        reader->log(event);

        read_go_next(reader);
    }
}

/**
 * Catch up all log readers to all events in the ring buffer
 */
static void sync_all_readers(void) {
    list_cursor_t cur = list_head(&readers);

    while(true) {
        log_reader_t *reader = list_cursor_entry(cur, log_reader_t, readers);

        if(reader == NULL) {
            break;
        }

        sync_reader(reader);        

        cur = list_cursor_next(cur);
    }
}

/**
 * Log a message
 * 
 * This is the common code for error(), warning() and info().
 * 
 * @param loglevel log message level/priority
 * @param source log source: JINUE_LOG_SOURCE_KERNEL or JINUE_LOG_SOURCE_USER
 * @param format printf-style format string for the message
 * @param args format string arguments
 */
static void log_message(int loglevel, int source, const char *restrict format, va_list args) {
    spin_lock(&logging_spinlock);

    if(pointer_is_in_redzone(RING_BUFFER_END)) {
        /* Not enough space is left at the end of the buffer, so we add a
         * terminator and continue writing from the start of the ring buffer.
         *
         * Before we do this, we need to get the tail pointer out of the
         * way. */
        push_tail();

        /* With the tail pointer out of the way, we add the terminator and
         * set the write pointer to the start of the ring buffer. */
        write_terminator();

        write_go_to_buffer_start();
    }

    /* Here, the write pointer is where we want it to be. Before we can write
     * the new event, we need to push the tail pointer if it is in the red
     * zone.
     * 
     * Edge case: when writing the very first event, the tail pointer is at the
     * beginning of the ring buffer and we want it to stay there to let the
     * write pointer overtake it. We will only start moving the tail pointer
     * once the write pointer has done a full round around the ring buffer. */
    if(state.write_id != 0) {
        push_tail();
    }

    log_event_t *event  = (log_event_t *)state.write_ptr;
    event->loglevel     = loglevel;
    event->source       = source;
    event->length       = vsnprintf(event->message, JINUE_LOG_MAX_LENGTH + 1, format, args);

    write_go_next();

    sync_all_readers();

    spin_unlock(&logging_spinlock);
}

/**
 * Log a message
 * 
 * @param loglevel log message level/priority
 * @param source source of log: JINUE_LOG_SOURCE_KERNEL or JINUE_LOG_SOURCE_USER
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

    log_message(JINUE_LOG_LEVEL_INFO, JINUE_LOG_SOURCE_KERNEL, format, args);

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

    log_message(JINUE_LOG_LEVEL_WARNING, JINUE_LOG_SOURCE_KERNEL, format, args);

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

    log_message(JINUE_LOG_LEVEL_ERROR, JINUE_LOG_SOURCE_KERNEL, format, args);

    va_end(args);
}
