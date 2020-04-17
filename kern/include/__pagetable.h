#ifndef ___PAGETABLE_H_
#define ___PAGETABLE_H_

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
    paddr_t entries[];
};

paddr_t pagetable_lookup(vaddr_t address);
int pagetable_set(vaddr_t address, int frame_no);
struct pagedirectory *pagedirectory_init();
void pagedirectory_cleanup(struct pagedirectory *pagedirectory);

#endif /* ___PAGETABLE_H_ */
