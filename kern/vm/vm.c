#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <addrspace.h>
#include <vm.h>
#include <machine/tlb.h>
#include <proc.h>
#include <spl.h>

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
	kprintf("\nFAULT @ 0x%08x : %s\n", faultaddress, faulttype == VM_FAULT_READONLY ? "VM_FAULT_READONLY" : (faulttype ==VM_FAULT_READ ? "VM_FAULT_READ": (faulttype ==VM_FAULT_WRITE?"VM_FAULT_WRITE":"???")));
    
    switch (faulttype) {
        case VM_FAULT_READONLY:
            return EFAULT;
        case VM_FAULT_READ:
        case VM_FAULT_WRITE:
            {
                struct addrspace *as;
                if ((as = proc_getas()) == NULL) {
                    panic("???");
                }

                // Step through the regions until we find the matching region
                struct region_node *node = as->regions.head;

                while (node != NULL) {
                    struct region *region = node->value;

                    // Check if region contains the fault address
                    if (region->vaddr > faultaddress || faultaddress >= (region->vaddr + region->memsize)) {
                        node = node->next;
                        continue;
                    }

                    kprintf("Matched region at 0x%08x-0x%08x: %s%s%s\n", region->vaddr, region->vaddr + region->memsize-1,
                    
                    region->readable ? "r" : "-",  region->writeable ? "w" : "-",  region->executable ? "x" : "-"
                    
                    );

                    // Get the addres holding the frame pointer
                    paddr_t *frameRef = pagetable_lookup(faultaddress);

                    // If the frame pointer is null, then it does not exist in the page table
                    if (*frameRef == (paddr_t) NULL) {
                        *frameRef = KVADDR_TO_PADDR(alloc_kpages(1));
                        kprintf("Allocated new page (physical) 0x%08x\n", *frameRef);
                    }

                    // EntryHi: 
                    //   20 bits - VPN
                    //   6 bits - ASID (ignore for OS161)  
                    //   6 bits - 000000
                    // EntryLo: 
                    //   20 bits - PFN
                    //   1 bit - Non-Cacheable (ignore for OS161)  
                    //   1 bit - Dirty -> Check value set from define address space
                    //   1 bit - Valid  
                    //   1 bit - Global (ignore for OS161 (FIXME: ???))
                    
                    // RWX -> DV conversion
                    // W -> D
                    // [Any] -> V
                    kprintf("\nVPN: 0x%08x\nPFN: 0x%08x\nDirty: %c\nValid: %c\n",faultaddress & PAGE_FRAME, *frameRef & PAGE_FRAME, (region->writeable &1) ? 'Y' : 'N', (region->readable || (region->writeable & 1) || region->executable) ? 'Y': 'N');
                    kprintf("EntryHi: 0x%08x\nEntryLo: 0x%08x\n",faultaddress & PAGE_FRAME, 
                    (*frameRef & PAGE_FRAME) 
                    | ((region->writeable & 1) ? TLBLO_DIRTY : 0) 
                    | ((region->readable || (region->writeable & 1) || region->executable) ? TLBLO_VALID : 0) 
                    );
                    int spl = splhigh();
                    tlb_random(faultaddress & PAGE_FRAME, 
                    (*frameRef & PAGE_FRAME) 
                    | ((region->writeable & 1) ? TLBLO_DIRTY : 0) 
                    | ((region->readable || (region->writeable & 1) || region->executable) ? TLBLO_VALID : 0) );
                    splx(spl);

                    return 0;
                }
            }

            return EFAULT;

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

