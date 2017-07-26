#ifndef JINUE_HAL_PAGE_TABLES_H
#define JINUE_HAL_PAGE_TABLES_H

#include <hal/types.h>


extern pte_t *(*get_pte_with_offset)(pte_t *, unsigned int);

extern void (*set_pte)(pte_t *, pfaddr_t, int);

extern void (*set_pte_flags)(pte_t *, int);

extern int (*get_pte_flags)(pte_t *);

extern pfaddr_t (*get_pte_pfaddr)(pte_t *);

extern void (*clear_pte)(pte_t *);

extern void (*copy_pte)(pte_t *, pte_t *);

void vm_x86_set_pte_pointers(void);

#endif
