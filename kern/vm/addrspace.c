/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <spl.h>
#include <spinlock.h>
#include <current.h>
#include <mips/tlb.h>
#include <addrspace.h>
#include <vm.h>
#include <proc.h>

/*
 * Note! If OPT_DUMBVM is set, as is the case until you start the VM
 * assignment, this file is not compiled or linked or in any way
 * used. The cheesy hack versions in dumbvm.c are used instead.
 *
 * UNSW: If you use ASST3 config as required, then this file forms
 * part of the VM subsystem.
 *
 */

/* */
static void __clear_tlb(void);

/*
 create a new empty address space. 
 
 You need to make sure this gets called in all the right places.  
 You may find you want to change the argument list.  
 May return NULL on out-of-memory error.
*/
struct addrspace *
as_create(void)
{
	struct addrspace *as;

	as = kmalloc(sizeof(struct addrspace));
	if (as == NULL) {
		return NULL;
	}

	as->pagedirectory = pagedirectory_init();
	as->regions = (struct region_container) {
		.head = NULL,
		.tail = NULL
	};

	/*
	 * Initialize as needed.
	 */
	kprintf("\nCHECKPOINTCHECKPOINTCHECKPOINTCHECKPOINTCHECKPOINTCHECKPOINT\n");
	kprintf("Address Space: %p", as);
	kprintf("\nPage Directory set to 0x%08x\n", (int) as->pagedirectory);
	kprintf("\nCHECKPOINTCHECKPOINTCHECKPOINTCHECKPOINTCHECKPOINTCHECKPOINT\n");
	return as;
}

/*
 create a new address space that is an exact copy of an old one.  
 
 Probably calls as_create to get a new empty address space and fill it in, but that's up to you.
*/
int
as_copy(struct addrspace *old, struct addrspace **ret)
{
	kprintf("\nCHECKPOINTCHECKPOINTCHECKPOINTCHECKPOINTCHECKPOINTCHECKPOINT\n");
	kprintf("BEGIN AS_COPY");
	kprintf("\nCHECKPOINTCHECKPOINTCHECKPOINTCHECKPOINTCHECKPOINTCHECKPOINT\n");

	struct addrspace *new_as;

	new_as = as_create();
	if (new_as==NULL) {
		return ENOMEM;
	}

	struct pagedirectory *old_pagedirectory = old->pagedirectory;
	struct pagedirectory *new_pagedirectory = new_as->pagedirectory;

	// Replicate page table structure
	struct pagetable **old_entries = old_pagedirectory->entries;
	struct pagetable **new_entries = new_pagedirectory->entries;

	for (int i = 0; i < PAGE_ENTRY_LIMIT; i++) {
		if (old_entries[i] != NULL) {
			struct pagetable *entry = kmalloc(sizeof(struct pagetable));
			// if (entry == NULL) {
			// 	as_destroy(new_as);
			// }
			memcpy(entry, &old_entries[i], sizeof(struct pagetable));
			new_entries[i] = entry;
		}
	}

	// Replicate region structure
	struct region_node *region_node = old->regions.head;
	struct region *region;
	while (region_node != NULL) {
		region = region_node->value;
		as_define_region(
			new_as,
			region->vaddr, 
			region->memsize, 
			region->readable, 
			region->writeable,
		 	region->executable
		);

		region_node = region_node->next;
	}

	*ret = new_as;
	return 0;
}



/*
 make curproc's address space the one currently "seen" by the processor.
*/
void
as_activate(void)
{
	struct addrspace *as;
	if ((as = proc_getas()) == NULL) {
		/*
		 * Kernel thread without an address space; leave the
		 * prior address space in place.
		 */
		return;
	}

	__clear_tlb();

}

/*
 unload curproc's address space so it isn't currently "seen" by the processor. 
  
 This is used to avoid potentially "seeing" it while it's being destroyed.
*/
void
as_deactivate(void)
{

	struct addrspace *as;
	if ((as = proc_getas()) == NULL) {
		/*
		 * Kernel thread without an address space; leave the
		 * prior address space in place.
		 */
		return;
	}
	/*
	 * Write this. For many designs it won't need to actually do
	 * anything. See proc.c for an explanation of why it (might)
	 * be needed.
	 */

	__clear_tlb();

}


/*
 dispose of an address space.  
 
 You may need to change the way this works if implementing user-level threads.
*/
void
as_destroy(struct addrspace *as)
{
	struct region_node *node = as->regions.head;
	while (node != NULL) {
		kfree(node->value);
		node = node->next;
	}

	pagedirectory_cleanup(as->pagedirectory);

	kfree(as);
}

/*
 set up a region of memory within the address space.
*/

/*
 * Set up a segment at virtual address VADDR of size MEMSIZE. The
 * segment in memory extends from VADDR up to (but not including)
 * VADDR+MEMSIZE.
 *
 * The READABLE, WRITEABLE, and EXECUTABLE flags are set if read,
 * write, or execute permission should be set on the segment. At the
 * moment, these are ignored. When you write the VM system, you may
 * want to implement them.
 */
int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t memsize,
		 int readable, int writeable, int executable)
{
	kprintf("\nCHECKPOINTCHECKPOINTCHECKPOINTCHECKPOINTCHECKPOINTCHECKPOINT\n");
	kprintf("BEGIN AS_DEFINE");
	kprintf("\nCHECKPOINTCHECKPOINTCHECKPOINTCHECKPOINTCHECKPOINTCHECKPOINT\n");


	// FIXME: :::
	writeable = 1;
	// FIXME: :::

	// TODO: Check if as is null!?

	// TODO: Lock???

	struct region *region;
	if ((region = kmalloc(sizeof(struct region))) == NULL) {
		panic("welp.");
	}

	*region = (struct region) {
		.vaddr = vaddr,
		.memsize = memsize,
		.readable = readable,
		.writeable = writeable,
		.executable = executable,
	};


	struct region_node *region_node;
	if ((region_node = kmalloc(sizeof(struct region_node))) == NULL) {
		panic("welp.");
	};

	*region_node = (struct region_node) {
		.value = region,
		.next = NULL
	};

	if (as->regions.tail != NULL) {
		as->regions.tail->next = region_node;
		as->regions.tail = region_node;
	} else {
		as->regions.head = as->regions.tail = region_node;
	}
	

	kprintf("\nCHECKPOINTCHECKPOINTCHECKPOINTCHECKPOINTCHECKPOINTCHECKPOINT\n");
	kprintf("Region assigned from 0x%08x to 0x%08x as %s%s%s", 
	vaddr, vaddr+memsize-1, readable ? "r" : "-",  writeable ? "w" : "-",  executable ? "x" : "-"
	);
	kprintf("\nCHECKPOINTCHECKPOINTCHECKPOINTCHECKPOINTCHECKPOINTCHECKPOINT\n");

	// FIXME: Return code
	return 0;
}

/* 
 this is called before actually loading from an executable into the address space.
*/
int
as_prepare_load(struct addrspace *as)
{
	// Code regions inside of the address are non-writable to the program
	// But we first need to write the program instructions into that space.
	// Mark read only regions as writeable

	struct region_node *node = as->regions.head;

	while (node != NULL) {
		node->value->writeable = 1;
		// FIXME: node->value->writeable = (node->value->writeable << 1) | 1;
		/*
		 00 -> 01
		 01 -> 11
		 |\ 
		 | Writable
		 Temp bit
		*/

		node = node->next;
	}
	kprintf("\nCHECKPOINTCHECKPOINTCHECKPOINTCHECKPOINTCHECKPOINTCHECKPOINT\n");
	kprintf("PREPARE LOAD");
	kprintf("\nCHECKPOINTCHECKPOINTCHECKPOINTCHECKPOINTCHECKPOINTCHECKPOINT\n");
	return 0;
}

/*
 this is called when loading from an executable is complete.
*/
int
as_complete_load(struct addrspace *as)
{
	kprintf("\nCHECKPOINTCHECKPOINTCHECKPOINTCHECKPOINTCHECKPOINTCHECKPOINT\n");
	kprintf("AS_COMPLETE_LOAD");
	kprintf("\nCHECKPOINTCHECKPOINTCHECKPOINTCHECKPOINTCHECKPOINTCHECKPOINT\n");
	struct region_node *node = as->regions.head;

	while (node != NULL) {
		node->value->writeable = 1;
		// FIXME:  node->value->writeable >>= 1;
		/*
		 01 -> 00
		 11 -> 01
		 |\ 
		 | Writable
		 Temp bit
		*/

		node = node->next;
	}

	return 0;
}

/*
 set up the stack region in the address space. 

 Normally called *after* as_complete_load().  
 Hands back the initial stack pointer for the new process.
*/
int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{

	// Define the stack as the last `USER_STACK_SIZE` bytes from USERSTACK
	// Allow Read and Write, but not Execute
  	int error = as_define_region(as, USERSTACK - USER_STACK_SIZE, USER_STACK_SIZE, 1, 1, 0);
    
	if (error) {
        return error;
    }

	/* Initial user-level stack pointer */
	*stackptr = USERSTACK;

	kprintf("\nCHECKPOINTCHECKPOINTCHECKPOINTCHECKPOINTCHECKPOINTCHECKPOINT\n");
	kprintf("STACK DEFINED");
	kprintf("\nCHECKPOINTCHECKPOINTCHECKPOINTCHECKPOINTCHECKPOINTCHECKPOINT\n");
	return 0;
}


static void __clear_tlb(void) {
	int spl = splhigh();
	for (int i = 0; i < 64; i++) {
		tlb_write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
	}
	splx(spl);
}
