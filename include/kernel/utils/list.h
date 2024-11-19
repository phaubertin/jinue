/*
 * Copyright (C) 2019-2024 Philippe Aubertin.
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

#ifndef JINUE_KERNEL_UTILS_LIST_H
#define JINUE_KERNEL_UTILS_LIST_H

#include <kernel/utils/utils.h>
#include <stdbool.h>
#include <stddef.h>

struct list_node_t {
    struct list_node_t *next;
};

typedef struct list_node_t list_node_t;

typedef struct  {
    list_node_t *head;
    list_node_t *tail;
} list_t;

typedef list_node_t **list_cursor_t;

#define STATIC_LIST   {.head = NULL, .tail = NULL}

static inline void init_list(list_t *list) {
    list->head = NULL;
    list->tail = NULL;
}

static inline void *list_node_entry_by_offset(list_node_t *node, size_t offset) {
    /* We specifically want to allow the return of list_dequeue() to be
     * passed to this function directly, i.e.
     * 
     *  foo_t *foo = list_node_entry(
     *          list_dequeue(some_list),
     *          foo_t,
     *          list_member);
     * 
     * This means handling the case where NULL is returned because there are no
     * items to dequeue in the list. */
    if(node == NULL) {
        return NULL;
    }
    
    return &((char *)node)[-offset];
}

#define list_node_entry(node, type, member) \
    ( (type *)list_node_entry_by_offset(node, OFFSET_OF(type, member)) )

static inline void list_enqueue(list_t *list, list_node_t *node) {
    /* no next node at the tail */
    node->next = NULL;
    
    /* if adding to an empty list... */
    if(list->tail == NULL) {
        /* ... the head is the same as the tail... */
        list->head = node;
    }
    else {
        /* ... otherwise, the old tail node's successor is the new tail node */
        list->tail->next = node;
    }
    
    /* add node at the tail */
    list->tail = node;
}

static inline list_node_t *list_dequeue_node(list_t *list) {
    list_node_t *node = list->head;
    
    if(node == NULL) {
        return NULL;
    }

    list->head = node->next;
    
    /* if removing the last node from the list ... */
    if(list->tail == node) {
        /*  ...update the tail pointer as well */
        list->tail = NULL;
    }
    
    return node;
}

#define list_dequeue(list, type, member) list_node_entry(list_dequeue_node(list), type, member)

static inline list_node_t *list_cursor_entry_by_offset(list_cursor_t cur, size_t offset) {
    return list_node_entry_by_offset(*cur, offset);
}

#define list_cursor_entry(cur, type, member) \
    ( (type *)list_cursor_entry_by_offset(cur, OFFSET_OF(type, member)) )

static inline list_cursor_t list_head(list_t *list) {
    return &(list->head);
}

static inline list_cursor_t list_cursor_next(list_cursor_t cur) {
    if(cur == NULL) {
        return NULL;
    }
    
    /* this assumes that the next pointer is the first struct member */
    return (list_cursor_t)(*cur);
}

#endif
