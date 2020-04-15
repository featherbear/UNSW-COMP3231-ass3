#include <__pagetable.h>
#include <types.h>
#include <vm.h>
#include <spinlock.h>
#include <lib.h>

#define PAGESIZE 1024

/* First level page table*/
struct base_pagetable
{
    struct spinlock lock;      // Global spinlock for ALL page table and level 2 page table operations
    struct pagetable *entries; // Array of page tables
};

/* Second level page table */
struct pagetable
{
    int n_entries;
    int *entries; // Should this be an array of entries
};

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
    bzero(pagetable->entries, 1024 * sizeof(int));

    return pagetable;
}

int pagetable_translate(int32_t *address)
{
    int32_t _address = (int32_t)address;

    // VIRTUAL_MEMORY_ADDRESS
    //   |       |       \ 
    //   |       \        12 bits - offset
    //   \       10 bits - level 2 offset
    //    10 bits - level 1 offset

    // FIXME: Check for Kernel address?

    int page_number = (_address & PAGE_FRAME) >> 12;
    int second_index = page_number & 0x3FF;
    int first_index = (page_number >> 10) & 0x3FF;
    int offset = _address & ~PAGE_FRAME;

    // int offset = _address & 0xFFF; // Last 12 bits
    // int index = (_address >>= 12) & 0x3FF; //
    // int table_index = (_address >>= 10) & 0x3FF;

    // TODO: Do lookup

    // return (lookup << 20) | offset;
    return 0;
}

/* 
 * Creates the first level page table. 
 */
int base_pagetable_init()
{
    struct base_pagetable *base_pagetable;
    if ((base_pagetable = kmalloc(1024 * sizeof(struct pagetable))) == NULL)
    {
        return NULL;
    }

    paddr_t highest_physical_addr = ram_getsize(); // Within vm.h 
    

    // welp I remember nothing
    // spinlock *base_pagetable_lock
    // TODO: Initialise spinlock
    // TODO: malloc space for 2^10 pagetable addresses ;; (RAMSIZE-FIRSTFREE) / PAGESIZE)
    // TODO: rip if malloc returned null
    // TODO: zero-fill
}