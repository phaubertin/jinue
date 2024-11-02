/*
 * Copyright (C) 2023 Philippe Aubertin.
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

#include <jinue/jinue.h>
#include <sys/mman.h>
#include <string.h>
#include "alloc.h"

#define STRING_AREA_SIZE    16384

#define DIRENT_AREA_SIZE    16384

/**
 * Allocate memory of specific size and up to next page boundary.
 *
 * Allocate enough memory for at least the specifies size. The underlying
 * allocator is mmap(), which means:
 *  - The address of the buffer is aligned on a page boundary
 *  - If the specified size is not a multiple of the page size, additional space
 *    is allocated up to the next page boundary.
 *
 * @param bytes size in bytes
 * @return address of allocated buffer, NULL on failure
 *
 * */
void *allocate_page_aligned(size_t bytes) {
    void *addr = mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);

    if(addr == MAP_FAILED) {
        return NULL;
    }

    return addr;
}

/**
 * Allocate memory from an allocation area.
 *
 * Enough space must remain in the area (not checked, must be checked by the
 * caller).
 *
 * @param area area from which to allocate
 * @param bytes size in bytes
 * @return address of allocated memory
 *
 * */
static void *allocate_from_area(alloc_area_t *area, size_t bytes) {
    char *addr = area->addr;

    area->addr = addr + bytes;
    area->bytes_remaining -= bytes;

    return addr;
}

/**
 * Allocate a buffer where it fits best in an array of allocation areas.
 *
 * Attempt to find the area with the smallest remaining size that will fit the
 * requested size. Allocate from that area if found. If no appropriate area is
 * found, this function fails without attempting to allocate new memory through
 * try_allocate() (unlike allocate_from_areas_best_fit()).
 *
 * @param areas array of areas from which to allocate
 * @param n_areas number of areas in the array
 * @param bytes size in bytes
 * @return address of allocated memory, NULL on failure
 *
 * */
static void *try_alloc_from_areas_best_fit(alloc_area_t *areas, size_t n_areas, size_t bytes) {
    alloc_area_t *area = NULL;

    for(int idx = 0; idx < n_areas; ++idx) {
        alloc_area_t *current = &areas[idx];

        if(current->bytes_remaining < bytes || current->addr == NULL) {
            continue;
        }

        if(area == NULL || current->bytes_remaining < area->bytes_remaining) {
            area = current;
        }
    }

    if(area == NULL) {
        return NULL;
    }

    return allocate_from_area(area, bytes);
}

/**
 * Find an area to evict.
 *
 * From an array of allocation areas, find the best area to replace with a
 * freshly allocated (i.e. bigger) block of memory. An area that has never been
 * set up with a block of memory (i.e. addr == NULL) is prime candidate.
 * Otherwise, the area with the smallest remaining allocatable size is chosen.
 *
 * @param areas array of areas
 * @param n_areas number of areas in the array
 * @return pointer to selected area
 *
 * */
static alloc_area_t *find_area_to_evict(alloc_area_t *areas, size_t n_areas) {
    alloc_area_t *area = &areas[0];

    for(int idx = 0; idx < n_areas; ++idx) {
        alloc_area_t *current = &areas[idx];

        if(current->addr == NULL) {
            return current;
        }

        if(current->bytes_remaining < area->bytes_remaining) {
            area = current;
        }
    }

    return area;
}

/**
 * Allocate a buffer where it fits best in an array of allocation areas.
 *
 * Attempt to find the area with the smallest remaining size that will fit the
 * requested size. It no area fits the requested size, allocate a fresh block of
 * memory and set up the area with the smallest remaining size to use it, then
 * allocate from there.
 *
 * The intent here is to attempt to reduce wasted memory slightly: once a given
 * block of memory (managed by an allocation area) is no longer large enough to
 * satisfy an allocation request, it is kept around for some time to possibly
 * satisfy a smaller request.
 *
 * Before the first call, the array must be initialized by clearing it with
 * memset().
 *
 * @param areas array of areas from which to allocate
 * @param n_areas number of areas in the array
 * @param bytes size in bytes
 * @return address of allocated memory, NULL on failure
 *
 * */
void *allocate_from_areas_best_fit(alloc_area_t *areas, size_t n_areas, size_t bytes) {
    void *addr = try_alloc_from_areas_best_fit(areas, n_areas, bytes);

    if(addr != NULL) {
        return addr;
    }

    alloc_area_t *area = find_area_to_evict(areas, n_areas);

    addr = allocate_page_aligned(STRING_AREA_SIZE);

    if(addr == NULL) {
        return NULL;
    }

    area->addr = addr;
    area->bytes_remaining = STRING_AREA_SIZE;

    return allocate_from_area(area, bytes);
}

/**
 * Allocate a new block to use for allocating directory entries.
 *
 * Allocate a block of the right size, then set up area to use it and add the
 * list terminator.
 *
 * @param area area from which to allocate directory entries
 * @return address of the list, NULL on failure
 *
 * */
static jinue_dirent_t *allocate_dirent_block(alloc_area_t *area) {
    jinue_dirent_t *terminator = allocate_page_aligned(DIRENT_AREA_SIZE);

    if(terminator == NULL) {
        return NULL;
    }

    memset(terminator, 0, DIRENT_AREA_SIZE);
    terminator->type        = JINUE_DIRENT_TYPE_END;

    /* For dirent areas, addr always points to the terminator, not after it,
     * even tough the terminator is counted as allocated space in bytes_remaining.
     * This makes is easier to find the terminator when appending. */
    area->addr              = terminator;
    area->bytes_remaining   = DIRENT_AREA_SIZE - sizeof(jinue_dirent_t);

    return terminator;
}

/**
 * Initialize an empty list of directory entries.
 *
 * In addition to allocating the data structures that represent the empty list,
 * this function sets up the allocation area so it can be used to add directory
 * entries to the list by passing it as an argument to append_dirent_to_list().
 *
 * @param area area from which to allocate directory entries
 * @return address of the list, NULL on failure
 *
 * */
jinue_dirent_t *initialize_empty_dirent_list(alloc_area_t *area) {
    return allocate_dirent_block(area);
}

/**
 * Allocate a new directory entry and append it to the list.
 *
 * Enough space must remain in the area (not checked, must be checked by the
 * caller).
 *
 * @param area area from which to allocate directory entries
 * @param type type of the directory entry (a JINUE_DIRENT_TYPE_... constant)
 * @return address of the directory entry
 *
 * */
static jinue_dirent_t *allocate_dirent(alloc_area_t *area, int type) {
    jinue_dirent_t *current     = area->addr;
    current->type               = type;

    jinue_dirent_t *terminator  = current + 1;
    terminator->type            = JINUE_DIRENT_TYPE_END;

    area->addr                  = terminator;
    area->bytes_remaining       -= sizeof(jinue_dirent_t);

    return current;
}

/**
 * Allocate a new directory entry and append it to the list.
 *
 * initialize_empty_dirent_list() must be called first to initialize area, and
 * then this function can be called as many times as needed to append directory
 * entries. The area structure maintains the state between calls.
 *
 * The directory entry is cleared (all zeroes) with the exception of the type
 * member, which is set to the type passed as argument.
 *
 * @param area area from which to allocate directory entries
 * @param type type of the directory entry (a JINUE_DIRENT_TYPE_... constant)
 * @return address of the directory entry, NULL on failure
 *
 * */
jinue_dirent_t *append_dirent_to_list(alloc_area_t *area, int type) {
    if(area->bytes_remaining >= sizeof(jinue_dirent_t)) {
        return allocate_dirent(area, type);
    }

    /* No more space in the current block, so we need to allocate a new one and
     * link to it. */
    jinue_dirent_t *link = area->addr;

    if(allocate_dirent_block(area) == NULL) {
        return NULL;
    }

    link->type      = JINUE_DIRENT_TYPE_NEXT;
    link->rel_value = (char *)area->addr - (char *)link;

    return allocate_dirent(area, type);
}
