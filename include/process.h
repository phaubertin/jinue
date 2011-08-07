#ifndef _JINUE_PROCESS_H
#define _JINUE_PROCESS_H

#include <slab.h>
#include <vm.h>

#define PROCESS_NAME_LENGTH 16

typedef unsigned long pid_t;

struct process_t {
	pid_t pid;
	addr_t cr3;
	struct process_t *next;
	char name[PROCESS_NAME_LENGTH];
};

typedef struct process_t process_t;

struct process_cb_t {
	process_t *p_descriptor;
};

typedef struct process_cb_t process_cb_t;

struct thread_t {
};

typedef struct thread_t thread_t;

extern pid_t next_pid;

extern process_t *first_process;

extern slab_cache_t process_slab_cache;

extern pte_t *page_directory_template;


/* defined in process.c */

process_t *process_create(void);

void process_destroy(process_t *p);

void process_destroy_by_pid(pid_t pid);

process_t *process_find_by_pid(pid_t pid);

/* defined in asm/process.asm */

void process_start(addr_t entry, addr_t stack);

#endif

