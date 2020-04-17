#include <__pagetable.h>
#include <vm.h>
#include <lib.h>

#define PG_LOCK_ACQUIRE() (spinlock_acquire(&pagedirectory->lock))
#define PG_LOCK_RELEASE() (spinlock_release(&pagedirectory->lock))



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

    // Assign page table references
    pagetable->entries = kmalloc(sizeof(/* 2^10 entries */ 1024 * sizeof(int)));
    kassert(pagetable->entries != NULL);
    bzero(pagetable->entries, 1024 * sizeof(int));

    return pagetable;
}

/* 
 * Translation from virtual address to physical address 
 */
paddr_t *pagetable_lookup(vaddr_t *address)
{
   return pagetable_lookup_tableref(address, NULL);
}

paddr_t *pagetable_lookup_tableref(vaddr_t *address, struct pagetable** tableref) {
    vaddr_t _address = (vaddr_t)address;

    // VIRTUAL_MEMORY_ADDRESS
    //   |       |       \ 
    //   |       \        12 bits - offset
    //   \       10 bits - level 2 offset
    //    10 bits - level 1 offset

    // FIXME: Check for Kernel address?

    int page_number = _address >> 12; // (_address & PAGE_FRAME) >> 12;
    int second_index = page_number & 0x3FF;
    int first_index = (page_number >> 10) & 0x3FF;

    int offset = _address & ~PAGE_FRAME;

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
        kassert(*pagetable_reference != NULL);
    }

    if (tableref != NULL) {
        *tableref = *pagetable_reference;
    }
    PG_LOCK_RELEASE();

    // Return pointer to frame value. May be null (in the event of frame not allocated)    
    return &((*pagetable_reference)->entries[second_index]);

    // // TODO: Assign frame if it does not exist???
    // if (*frame == NULL) // i.e. pagetable->entries[first_index]->entries[second_index] is null
    // {
    //     (*pagetable_reference)->n_entries++;
    //     // TODO: Assign a frame // *frame
    // }

    // return ((PAGE_SIZE * *frame) << 20) | offset;
}

int pagetable_set(vaddr_t *address, int frame_no) {
    struct pagetable *table = NULL;
    int *frame_reference = pagetable_lookup_tableref(address, &table);
    PG_LOCK_ACQUIRE();
    if (*frame_reference == NULL) {
        table->n_entries++;
    }
    PG_LOCK_RELEASE();

    *frame_reference = frame_no;
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
    // paddr_t highest_physical_addr = ram_getsize();
    // paddr_t lowest_physical_addr = ram_getfirstfree();
    // unsigned int n_entries = (highest_physical_addr - lowest_physical_addr) / PAGE_SIZE;

    if ((pagedirectory->entries = kmalloc(1024 * sizeof(struct pagetable *))) == NULL)
    {
        spinlock_cleanup(&pagedirectory->lock);
        return NULL;
    }

    bzero(pagedirectory->entries, 1024 * sizeof(struct pagetable *));

    // Success
    return pagedirectory;
}

void pagedirectory_cleanup(struct pagedirectory *pagedirectory) {
    // TODO: Release the page directory entries
    // Also free the frametable pages??? - What about shared frames???

}