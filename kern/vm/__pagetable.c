#include <__pagetable.h>
#include <types.h>
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
int pagetable_translate(int32_t *address)
{
    int32_t _address = (int32_t)address;

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

    // Search for frame index
    int *frame = &((*pagetable_reference)->entries[second_index]);

    // TODO: Assign frame if it does not exist???
    if (*frame == NULL) // i.e. pagetable->entries[first_index]->entries[second_index] is null
    {
        (*pagetable_reference)->n_entries++;
        // TODO: Assign a frame // *frame
    }
    PG_LOCK_RELEASE();

    return ((PAGE_SIZE * *frame) << 20) | offset;
}

/* 
 * Creates the first level page table. 
 */
struct pagedirectory *pagedirectory_init()
{
    struct pagedirectory *pagedirectory = NULL;

    if ((pagedirectory = kmalloc(sizeof(struct pagetable))) == NULL)
    {
        return NULL;
    }

    spinlock_init(&pagedirectory->lock);

    paddr_t highest_physical_addr = ram_getsize();
    paddr_t lowest_physical_addr = ram_getfirstfree();
    unsigned int n_entries = (highest_physical_addr - lowest_physical_addr) / PAGE_SIZE;

    if ((pagedirectory->entries = kmalloc(sizeof(struct pagetable) * n_entries)) == NULL)
    {
        return 1;
    }

    bzero(pagedirectory->entries, n_entries * sizeof(struct pagetable));

    // Success
    return pagedirectory;
}

/*

Jennifer's Thought Bubble

try {
    do (bubble.grow()) while true;
} catch {
    bubble.pop();
}

*/