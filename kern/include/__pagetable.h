#ifndef _PAGETABLE_H_
#define _PAGETABLE_H_

#include <types.h>
#include <spinlock.h>


/* Page directory */
struct pagedirectory
{
    struct spinlock lock;        // Global spinlock for ALL page table and level 2 page table operations
    struct pagetable *entries[]; // Array of page tables
};

/* Second level page table */
struct pagetable
{
    int n_entries;
    int entries[];
};

int *pagetable_lookup(vaddr_t *address);
int pagetable_set(vaddr_t *address, int frame_no);


#endif /* _PAGETABLE_H_ */
