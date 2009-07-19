#include <process.h>
#include <slab.h>
#include <stddef.h>
#include <vm.h>
#include <vm_alloc.h>

/** PID for next process creation */
pid_t next_pid;

/** head of process descriptors linked list */
process_t *first_process;

/** slab cache for allocation of process descriptors */
slab_cache_t process_slab_cache;

/** template for the creation of a new page directory */
pte_t *page_directory_template;


process_t *process_create(void) {
	process_t *p = slab_alloc(&process_slab_cache);
	
	while( process_find_by_pid(next_pid) != NULL ) {
		++next_pid;
	}
	
	p->pid = next_pid++;
	
	/* TODO: actual implementation */
	return p;
}

void process_destroy(process_t *p) {
	/* TODO: actual implementation */
}

void process_destroy_by_pid(pid_t pid) {
	process_destroy( process_find_by_pid(pid) );
}

process_t *process_find_by_pid(pid_t pid) {
	process_t *p;
	
	p = first_process;
	
	while(p != NULL) {
		if(p->pid == pid) {
			return p;
		}
		
		p = p->next;
	}
	
	return NULL;
}

