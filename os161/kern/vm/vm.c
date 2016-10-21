#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <curthread.h>
#include <addrspace.h>
#include <vm.h>
#include <machine/spl.h>
#include <machine/tlb.h>
#include <synch.h>

/*
 * Smart MIPS-only "VM system" that is intended to be amazing
 * enough to fly off the ground.
 */

/* under smartvm, always have 48k of user stack */
 #define SMARTVM_STACKPAGES    1

struct coremap_struct *coremap;
paddr_t firstpaddr, lastpaddr, freepaddr;
static struct lock *coremap_lock;
static int after_vm_bootstrap;

void
vm_bootstrap(void)
{
	int total_pages, i;
	paddr_t curpaddr;

	coremap_lock = NULL;

	if (coremap_lock == NULL) {       // Create the coremap lock
        coremap_lock = lock_create("coremap_lock");
        if (coremap_lock == NULL) {
            panic("coremap: lock_create failed\n");
        }
    }
	
	ram_getsize(&firstpaddr, &lastpaddr);
	
	total_pages = (lastpaddr - firstpaddr)/PAGE_SIZE;

	//kprintf("Number of pages: %d\n", total_pages);
	
	freepaddr = firstpaddr + total_pages * sizeof(struct coremap_struct);

	//kprintf("First address: 0x%x\n", firstpaddr);
	//kprintf("Last address: 0x%x\n", lastpaddr);
	//kprintf("Free address: 0x%x\n", freepaddr);

	coremap = (struct coremap_struct*)PADDR_TO_KVADDR(firstpaddr);

	curpaddr = firstpaddr;

	i = 0;

	while(curpaddr < lastpaddr){

		coremap[i].pa = curpaddr;
		coremap[i].va = PADDR_TO_KVADDR(curpaddr);

		//kprintf("%d. Pa address: 0x%x\n", i, coremap[i].pa);
		//kprintf("%d. Va address: 0x%x\n", i, coremap[i].va);

		if(curpaddr < freepaddr){
			coremap[i].state = FIXED;
		}else{
			coremap[i].state = FREE;
		}

		curpaddr += PAGE_SIZE;
		i++;
	}

	after_vm_bootstrap = 1;
}

paddr_t
getppages(unsigned long npages)
{
	paddr_t addr;
	int time_waster = 1000;
	int i;

	for(i = 0; i < time_waster; i++);

	//kprintf("In getppages, npages = %d\n", npages);
	if(!after_vm_bootstrap){
		//kprintf("Not bootstrapped\n");
		addr = ram_stealmem(npages);
	}else{
		//kprintf("Bootstrapped\n");
		lock_acquire(coremap_lock);
		//kprintf("After lock\n");

		int total_pages, start_spot;
		unsigned long n_pages = 0;
		int i = 0;
		total_pages = (lastpaddr - firstpaddr)/PAGE_SIZE;
		//kprintf("total_pages = %d\n", total_pages);

		while(i < total_pages){
			if(coremap[i].state == FREE){
				n_pages++;
				if(n_pages == npages){
					start_spot = i - n_pages + 1;
					break;
				}
			}else{
				n_pages = 0;
			}
			i++;
		}
		//kprintf("After while, i = %d, start_spot = %d\n", i, start_spot);

		if(i == total_pages){
			lock_release(coremap_lock);
			return 0;
		}

		int j = 0;
		while(j < npages){
			coremap[j + start_spot].state = FIXED;
			coremap[j + start_spot].start_of_group = coremap[start_spot].va;
			j++;
		}

		addr = coremap[start_spot].pa;
		
		//kprintf("Address: 0x%x\n", addr);

		lock_release(coremap_lock);
	}
	
	return addr;
}

/* Allocate/free some kernel-space virtual pages */
vaddr_t 
alloc_kpages(int npages)
{
	paddr_t pa;
	//kprintf("In alloc_kpages\n");
	pa = getppages(npages);
	if (pa==0) {
		return 0;
	}
	return PADDR_TO_KVADDR(pa);
}

void 
free_kpages(vaddr_t addr)
{
	//kprintf("In free_kpages\n");
	int total_pages;
	vaddr_t start_of_group;
	int i = 0;
	int j = 0;
	total_pages = (lastpaddr - firstpaddr)/PAGE_SIZE;

	while(i != total_pages){
		if(addr == coremap[i].va){
			start_of_group = coremap[i].start_of_group;
			for(j = 0; j < total_pages; j++){
				if(coremap[j].start_of_group == start_of_group){
					coremap[j].state = FREE;
					//kprintf("Free page: %d\n", j);
				}
			}
			break;
		}
		i++;
	}

	return;
}

int
vm_fault(int faulttype, vaddr_t faultaddress)
{
	vaddr_t vbase1, vtop1, vbase2, vtop2, stackbase, stacktop, heap_top, heap_base;
	paddr_t paddr = -1;
	int i = 0;
	u_int32_t ehi, elo;
	struct addrspace *as;
	int spl;

	spl = splhigh();

	faultaddress &= PAGE_FRAME;
	

	DEBUG(DB_VM, "smartvm: fault: 0x%x\n", faultaddress);

	switch (faulttype) {
	    case VM_FAULT_READONLY:
		/* We always create pages read-write, so we can't get this */
		panic("smartvm: got VM_FAULT_READONLY\n");
	    case VM_FAULT_READ:
	    case VM_FAULT_WRITE:
		break;
	    default:
		splx(spl);
		return EINVAL;
	}

	as = curthread->t_vmspace;
	if (as == NULL) {
		/*
		 * No address space set up. This is probably a kernel
		 * fault early in boot. Return EFAULT so as to panic
		 * instead of getting into an infinite faulting loop.
		 */
		return EFAULT;
	}

	/* Assert that the address space has been set up properly. */
	assert(as->as_regions->vaddr != 0);
	assert(as->as_regions->paddr != 0);
	assert(as->as_regions->num_pages != 0);
	assert(as->as_regions->next->vaddr != 0);
	assert(as->as_regions->next->paddr!= 0);
	assert(as->as_regions->next->num_pages != 0);
	assert(as->as_stackpbase != 0);
	assert((as->as_regions->vaddr & PAGE_FRAME) == as->as_regions->vaddr);
	assert((as->as_regions->paddr & PAGE_FRAME) == as->as_regions->paddr);
	assert((as->as_regions->next->vaddr & PAGE_FRAME) == as->as_regions->next->vaddr);
	assert((as->as_regions->next->paddr & PAGE_FRAME) == as->as_regions->next->paddr);
	assert((as->as_stackpbase & PAGE_FRAME) == as->as_stackpbase);

	vbase1 = as->as_regions->vaddr;
	vtop1 = vbase1 + as->as_regions->num_pages * PAGE_SIZE;
	vbase2 = as->as_regions->next->vaddr;
	vtop2 = vbase2 + as->as_regions->next->num_pages * PAGE_SIZE;
	stackbase = USERSTACK - SMARTVM_STACKPAGES * PAGE_SIZE;
	stacktop = USERSTACK;
	heap_base = as->heap_start;
	heap_top = as->heap_end;

	if (faultaddress >= vbase1 && faultaddress < vtop1) {
		paddr = (faultaddress - vbase1) + as->as_regions->paddr;
	}
	else if (faultaddress >= vbase2 && faultaddress < vtop2) {
		paddr = (faultaddress - vbase2) + as->as_regions->next->paddr;
	}
	else if (faultaddress >= stackbase && faultaddress < stacktop) {
		paddr = (faultaddress - stackbase) + as->as_stackpbase;
	}
	else if (faultaddress >= heap_base && faultaddress < heap_top) {
		paddr = (faultaddress - heap_base) + as->heap->paddr;
	}else {
		splx(spl);
		return EFAULT;
	}

	/* make sure it's page-aligned */
	assert((paddr & PAGE_FRAME)==paddr);

	for (i=0; i<NUM_TLB; i++) {
		TLB_Read(&ehi, &elo, i);
		if (elo & TLBLO_VALID) {
			continue;
		}
		ehi = faultaddress;
		elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
		DEBUG(DB_VM, "smartvm: 0x%x -> 0x%x\n", faultaddress, paddr);
		TLB_Write(ehi, elo, i);
		splx(spl);
		return 0;
	}

	kprintf("smartvm: Ran out of TLB entries - cannot handle page fault\n");
	splx(spl);
	return EFAULT;
}
