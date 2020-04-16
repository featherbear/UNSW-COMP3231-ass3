#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <addrspace.h>
#include <vm.h>
#include <machine/tlb.h>

/* Place your page table functions here */


void vm_bootstrap(void)
{
    /* Initialise any global components of your VM sub-system here.  
     *  
     * You may or may not need to add anything here depending what's
     * provided or required by the assignment spec.
     */
}

int
vm_fault(int faulttype, vaddr_t faultaddress)
{
    switch (faulttype) {
        case VM_FAULT_READONLY:
            return EFAULT;
        case VM_FAULT_READ:
        case VM_FAULT_WRITE:

            int *frameRef = pagetable_lookup(faultaddress);

            if (*frameRef == NULL) {
                // Does not exist in the Page Table
                *frameRef = KVADDR_TO_PADDR(alloc_kpages(1));
            }

            // EntryHi: 
            //   20 bits - VPN
            //   6 bits - ASID (ignore for OS161)  
            //   6 bits - 000000
            // EntryLo: 
            //  20 bits - PFN
            //  1 bit - Non-Cacheable (ignore for OS161)  
            //  1 bit - Dirty -> Check value set from define address space
            //  1 bit - Valid  
            //  1 bit - Global

            // TODO: Step through as->regions->head     ->next     ->next    ->next etc
            // until we find the matching region
            // then compare RWX againt DV
            TBLO_VALID  region->readable || region->writeable || region->executable

            tlb_random(faultaddress & PAGE_FRAME, (*frameRef & PAGE_FRAME) | TLBLO_DIRTY | TLBLO_VALID /* TODO: FIXME */);

            // On successful allocation, return 0            
            return 0;
        default:
            return EINVAL;
    }

    

    return EFAULT;
}

/*
 * SMP-specific functions.  Unused in our UNSW configuration.
 */

void
vm_tlbshootdown(const struct tlbshootdown *ts)
{
	(void)ts;
	panic("vm tried to do tlb shootdown?!\n");
}

