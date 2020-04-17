#ifndef ___PAGETABLE_H_
#define ___PAGETABLE_H_

#include <types.h>
#include <spinlock.h>


/* Page directory */
struct pagedirectory
{
    struct spinlock lock;        // Global spinlock for ALL page table and level 2 page table operations
    struct pagetable **entries;  // Array of page tables
};

/* Second level page table */
struct pagetable
{
    int n_entries;     // NOT USED
    paddr_t *entries;
};

paddr_t* pagetable_lookup(vaddr_t);
paddr
struct pagedirectory *pagedirectory_init(void);
void pagedirectory_cleanup(struct pagedirectory *);

#define PAGE_ENTRY_LIMIT 1024 /* 2^10 == 1024 */

#endif /* ___PAGETABLE_H_ */
