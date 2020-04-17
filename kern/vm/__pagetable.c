#include <__pagetable.h>
#include <vm.h>
#include <lib.h>
#include <addrspace.h>
#include <proc.h>
#include <current.h>

#define PG_LOCK_ACQUIRE() (spinlock_acquire(&(proc_getas()->pagedirectory->lock)))
#define PG_LOCK_RELEASE() (spinlock_release(&(proc_getas()->pagedirectory->lock)))


struct pagetable *create_pagetable(void);
paddr_t* pagetable_lookup_tableref(vaddr_t, struct pagetable **);

/*
 * Creates a level 2 page table.
 * 
 * To be called when a page fault to a level 2 table occurs
 */
struct pagetable *create_pagetable()
{

    struct pagetable *pagetable;

    if ((pagetable = kmalloc(sizeof(struct pagetable))) == NULL)
    {
        return NULL;
    }


    pagetable->n_entries = 0;

    // Zero the page entries
    pagetable->entries = kmalloc(PAGE_ENTRY_LIMIT * sizeof(paddr_t));
    bzero(pagetable->entries, PAGE_ENTRY_LIMIT * sizeof(paddr_t));

    return pagetable;
}

/* 
 * Translation from virtual address to physical address 
 */
paddr_t* pagetable_lookup(vaddr_t address)
{
   return pagetable_lookup_tableref(address, NULL);
}

paddr_t* pagetable_lookup_tableref(vaddr_t address, struct pagetable** tableref) {

    struct pagedirectory *pagedirectory = proc_getas()->pagedirectory;
    /*
     VIRTUAL_MEMORY_ADDRESS
       |       |       \ 
       |       \        12 bits - offset
       \       10 bits - level 2 offset
        10 bits - level 1 offset
    */

    // FIXME: Check for Kernel address?
    
    int page_number = address >> 12; // (_address & PAGE_FRAME) >> 12;
    int second_index = page_number & 0x3FF;
    int first_index = (page_number >> 10) & 0x3FF;


    // int offset = address & ~PAGE_FRAME;

    // int offset = _address & 0xFFF; // Last 12 bits
    // int index = (_address >>= 12) & 0x3FF; //
    // int table_index = (_address >>= 10) & 0x3FF;

    // Search for level 2 page table
    struct pagetable **pagetable_reference = &pagedirectory->entries[first_index];


    PG_LOCK_ACQUIRE();
    
    // Create level 2 pagetable if it does not exist
    if (*pagetable_reference == NULL)
    {
        *pagetable_reference = create_pagetable();
        KASSERT(*pagetable_reference != NULL);
    }

    if (tableref != NULL) {
        *tableref = *pagetable_reference;
    }

    PG_LOCK_RELEASE();
    
    // Return pointer to frame value. May be null (in the event of frame not allocated)    
    return &((*pagetable_reference)->entries[second_index]);

}

int pagetable_set(vaddr_t address, paddr_t addr) {
    struct pagetable *table = NULL;
    paddr_t *frame_reference = pagetable_lookup_tableref(address, &table);

    PG_LOCK_ACQUIRE();
    if (*frame_reference == (paddr_t) NULL) {
        table->n_entries++;
    }
    PG_LOCK_RELEASE();

    *frame_reference = addr;

    return 0;
}
/* 
 * Creates the first level page table. 
 */
struct pagedirectory *pagedirectory_init()
{
    struct pagedirectory *pagedirectory = NULL;

    if ((pagedirectory = kmalloc(sizeof(struct pagedirectory))) == NULL)
    {
        return NULL;
    }

    spinlock_init(&pagedirectory->lock);


    // FIXME: Don't need this - it's already implemented in unsw.c
    // FIXME: paddr_t highest_physical_addr = ram_getsize();
    // FIXME: paddr_t lowest_physical_addr = ram_getfirstfree();
    // FIXME: unsigned int n_entries = (highest_physical_addr - lowest_physical_addr) / PAGE_SIZE;

    if ((pagedirectory->entries = kmalloc(PAGE_ENTRY_LIMIT * sizeof(struct pagetable *))) == NULL)
    {
        spinlock_cleanup(&pagedirectory->lock);
        return NULL;
    }

    bzero(pagedirectory->entries, PAGE_ENTRY_LIMIT * sizeof(struct pagetable *));

    // Success

    return pagedirectory;
}

void pagedirectory_cleanup(struct pagedirectory *pagedirectory) {
    // Also free the frametable pages??? - What about shared frames???
    
    if (pagedirectory->entries != NULL) {
        for (int i = 0; i < PAGE_ENTRY_LIMIT; i++) {
            kfree(pagedirectory->entries[i]);
        }
    }
    kfree(pagedirectory->entries);

    spinlock_cleanup(&pagedirectory->lock);
}