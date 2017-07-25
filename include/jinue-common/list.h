#ifndef _JINUE_COMMON_LIST_H
#define _JINUE_COMMON_LIST_H

#include <stdbool.h>
#include <stddef.h>

struct jinue_node_t {
    struct jinue_node_t *next;
};

typedef struct jinue_node_t jinue_node_t;

static inline void jinue_node_init(jinue_node_t *node) {
#ifndef NDEBUG
    /* A node initializer function is not strictly necessary because a node is
     * (re)-initialized when it is added to a list. When compiling in debug mode,
     * this function initializes a node by putting a recognizable value in the
     * next member so initialization bugs are easier to track. When not in debug
     * mode, this function compiles to nothing. */
    node->next = (jinue_node_t *)0xdeadbeef;
#endif
}

typedef struct  {
    jinue_node_t   *head;
    jinue_node_t   *tail;
} jinue_list_t;

typedef jinue_node_t **jinue_cursor_t;

#define JINUE_LIST_STATIC   {.head = NULL, .tail = NULL}

static inline void jinue_list_init(jinue_list_t *list) {
    list->head = NULL;
    list->tail = NULL;
}

static inline bool jinue_list_is_empty(const jinue_list_t *list) {
    return list->head == NULL;
}

static inline void jinue_list_enqueue(jinue_list_t *list, jinue_node_t *node) {
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

static inline void jinue_list_push(jinue_list_t *list, jinue_node_t *node) {
    /* add to the head */
    node->next = list->head;
    list->head = node;
    
    /* if adding to an empty list... */
    if(list->tail == NULL) {
        /* ... the tail is the same as the head */
        list->tail = node;
    }
}

static inline jinue_node_t *jinue_list_dequeue(jinue_list_t *list) {
    jinue_node_t *node = list->head;
    
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

#define jinue_list_pop(l)   ( jinue_list_dequeue((l)) )

static inline void *jinue_node_entry_by_offset(jinue_node_t *node, size_t offset) {
    /* We specifically want to allow the return of jinue_list_dequeue() to be
     * passed to this function directly, i.e.
     * 
     *  foo_t *foo = jinue_node_entry(
     *          jinue_list_dequeue(some_list),
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

/** TODO move this to a more general-purpose header file */
#define JINUE_OFFSETOF(type, member) ((size_t)(&((type *)0)->member))

#define jinue_node_entry(node, type, member)   (jinue_node_entry_by_offset(node, JINUE_OFFSETOF(type, member)))

static inline jinue_node_t *jinue_cursor_node(jinue_cursor_t cur) {
    if(cur == NULL) {
        return NULL;
    }
    
    return *cur;
}

static inline jinue_node_t *jinue_cursor_entry_by_offset(jinue_cursor_t cur, size_t offset) {
    return jinue_node_entry_by_offset(*cur, offset);
}

#define jinue_cursor_entry(cur, type, member)   (jinue_cursor_entry_by_offset(cur, JINUE_OFFSETOF(type, member)))

static inline jinue_cursor_t jinue_list_head_cursor(jinue_list_t *list) {
    return &(list->head);
}

static inline jinue_cursor_t jinue_cursor_next(jinue_cursor_t cur) {
    if(cur == NULL) {
        return NULL;
    }
    
    /* this assumes that the next pointer is the first struct member */
    return (jinue_cursor_t)(*cur);
}

static inline jinue_cursor_t jinue_circular_insert_before(jinue_cursor_t cur, jinue_node_t *node) {
    /* if the list is initially empty ... */
    if(cur == NULL) {
        /* ... node is alone in the list, so it is its own successor */
        node->next = node;
        
        return &node->next;
    }
    
    /* set sucessor of added node */
    node->next = (*cur);
    
    /* link node */
    (*cur) = node;
    
    /* cur now refers to the newly added node, so advance by one to refer to
     * the same node as before the insertion */
    return (jinue_cursor_t)(*cur);
}

static inline jinue_cursor_t jinue_circular_insert_after(jinue_cursor_t cur, jinue_node_t *node) {
    /* if the list is initially empty ... */
    if(cur == NULL) {
        /* ... node is alone in the list, so it is its own successor */
        node->next = node;
        
        return &node->next;
    }
    
    /* set sucessor of added node */
    node->next = (*cur)->next;
    
    /* link node */
    (*cur)->next = node;
    
    return cur;
}

static inline jinue_cursor_t jinue_circular_remove(jinue_cursor_t cur) {
    /* if the node referenced by the cursor is its own successor, it is
     * the only node in the list ... */
     if(cur == NULL || (*cur) == (*cur)->next) {
         /* ... so the list becomes empty */
         return NULL;
     }
     
     /* unlink the node to which the cursor refers */
     *cur = (*cur)->next;
     
     return cur;
}

#endif
