#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <curthread.h>
#include <addrspace.h>
#include <vm.h>
#include <machine/spl.h>
#include <machine/tlb.h>

/*
 * Note! If OPT_DUMBVM is set, as is the case until you start the VM
 * assignment, this file is not compiled or linked or in any way
 * used. The cheesy hack versions in dumbvm.c are used instead.
 */

/* under smartvm, always have 48k of user stack */
 #define SMARTVM_STACKPAGES    1

struct addrspace *
as_create(void)
{
	//kprintf("as_create\n");

	struct addrspace *as = kmalloc(sizeof(struct addrspace));
	if (as==NULL) {
		return NULL;
	}

	as->as_stackpbase = 0;
	as->heap_start = 0;
	as->heap_end = 0;
    as->as_regions = NULL;
    as->heap = NULL;

	return as;
}

int
as_copy(struct addrspace *old, struct addrspace **ret)
{

	struct addrspace *new;
	struct region_wrapper *temp;

	new = as_create();
	if (new==NULL) {
		return ENOMEM;
	}

	new->as_regions = (struct region_wrapper *)kmalloc(sizeof(struct region_wrapper));
	new->as_regions->permissions = old->as_regions->permissions;
	new->as_regions->vaddr = old->as_regions->vaddr;
	new->as_regions->num_pages = old->as_regions->num_pages;

	temp = (struct region_wrapper *)kmalloc(sizeof(struct region_wrapper));
	temp->permissions = old->as_regions->next->permissions;
	temp->vaddr = old->as_regions->next->vaddr;
	temp->num_pages = old->as_regions->next->num_pages;
	temp->next = NULL;

	new->as_regions->next = temp;

	new->heap_start = temp->vaddr + (temp->num_pages * PAGE_SIZE);
	new->heap_end = new->heap_start;

	if (as_prepare_load(new)) {
		as_destroy(new);
		return ENOMEM;
	}

	memmove((void *)PADDR_TO_KVADDR(new->as_regions->paddr),
		(const void *)PADDR_TO_KVADDR(old->as_regions->paddr),
		old->as_regions->num_pages*PAGE_SIZE);

	memmove((void *)PADDR_TO_KVADDR(new->as_regions->next->paddr),
		(const void *)PADDR_TO_KVADDR(old->as_regions->next->paddr),
		old->as_regions->next->num_pages*PAGE_SIZE);

	memmove((void *)PADDR_TO_KVADDR(new->as_stackpbase),
		(const void *)PADDR_TO_KVADDR(old->as_stackpbase),
		SMARTVM_STACKPAGES*PAGE_SIZE);
	
	*ret = new;
	return 0;
}

void
as_destroy(struct addrspace *as)
{	
	//kprintf("as_destroy\n");

	struct region_wrapper *leader;
	struct region_wrapper *follower;
	struct pte *pte_leader;
	struct pte *pte_follower;
	leader = as->as_regions;
	pte_leader = as->heap;
	if(leader != NULL){
		while(leader->next != NULL){
			follower = leader;
			leader = leader->next;
			free_kpages(PADDR_TO_KVADDR(follower->paddr));
			kfree(follower);
		}
		free_kpages(PADDR_TO_KVADDR(leader->paddr));
		kfree(leader);
	}
	
	if(pte_leader != NULL){
		while(pte_leader->next != NULL){
			pte_follower = pte_leader;
			pte_leader = pte_leader->next;
			free_kpages(pte_follower->vaddr);
			kfree(pte_follower);
		}
		free_kpages(pte_leader->vaddr);
		kfree(pte_leader);
	}

	free_kpages(PADDR_TO_KVADDR(as->as_stackpbase));

	kfree(as);
}

void
as_activate(struct addrspace *as)
{
	/*
	 * Write this.
	 */

	 int i, spl;

	 (void)as;

	 spl = splhigh();

	 for (i=0; i<NUM_TLB; i++) {
	 	TLB_Write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
	 }

	 splx(spl);
}

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
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t sz,
		 int readable, int writeable, int executable)
{

	size_t npages;
	struct region_wrapper *temp;
	struct region_wrapper *adding;

	/* Align the region. First, the base... */
	sz += vaddr & ~(vaddr_t)PAGE_FRAME;
	vaddr &= PAGE_FRAME;

	/* ...and now the length. */
	sz = (sz + PAGE_SIZE - 1) & PAGE_FRAME;

	npages = sz / PAGE_SIZE;

	if(as->as_regions == NULL){
		as->as_regions = (struct region_wrapper *)kmalloc(sizeof(struct region_wrapper));
    	as->as_regions->permissions = 7 & (readable | writeable | executable);
		as->as_regions->vaddr = vaddr;
		as->as_regions->num_pages = npages;
		as->heap_start = vaddr + (npages * PAGE_SIZE);
		as->heap_end = as->heap_start;
		as->as_regions->next = NULL;
		//kprintf("\n1: as_define:\nas->heap_start: 0x%x\nas->heap_end:0x%x\nas->as_regions->permissions:%d\nvaddr:0x%x\npaddr:0x%x\nallocated:%d\nnum_pages:%d\n", as->heap_start, as->heap_end, as->as_regions->permissions, as->as_regions->vaddr, as->as_regions->paddr, as->as_regions->allocated, as->as_regions->num_pages);
		return 0;
	}

	temp = as->as_regions;

	if(temp->next == NULL){
		adding = (struct region_wrapper *)kmalloc(sizeof(struct region_wrapper));
		adding->permissions = 7 & (readable | writeable | executable);
		adding->vaddr = vaddr;
		adding->num_pages = npages;
		adding->next = NULL;
		temp->next = adding;
		as->heap_start = vaddr + (npages * PAGE_SIZE);
		as->heap_end = as->heap_start;
		//kprintf("\n2: as_define:\nas->heap_start: 0x%x\nas->heap_end:0x%x\nas->as_regions->permissions:%d\nvaddr:0x%x\npaddr:0x%x\nallocated:%d\nnum_pages:%d\n", as->heap_start, as->heap_end, temp->permissions, temp->vaddr, temp->paddr, temp->allocated, temp->num_pages);
		return 0;
	}

	kprintf("smartvm: Warning: too many regions\n");
	return EUNIMP;
}

int
as_prepare_load(struct addrspace *as)
{
	/*
	 * Write this.
	 */

	//kprintf("as_prepare_load\n");

	as->as_regions->paddr = getppages(as->as_regions->num_pages);
	if (as->as_regions->paddr == 0) {
		return ENOMEM;
	}
	as->as_regions->allocated++;
	//kprintf("\n1: as_prepare_load:\nas->heap_start: 0x%x\nas->heap_end:0x%x\nas->as_regions->permissions:%d\nvaddr:0x%x\npaddr:0x%x\nallocated:%d\nnum_pages:%d\n", as->heap_start, as->heap_end, as->as_regions->permissions, as->as_regions->vaddr, as->as_regions->paddr, as->as_regions->allocated, as->as_regions->num_pages);	

	as->as_regions->next->paddr = getppages(as->as_regions->next->num_pages);
	if (as->as_regions->next->paddr == 0) {
		return ENOMEM;
	}
	as->as_regions->next->allocated++;
	//kprintf("\n2: as_prepare_load:\nas->heap_start: 0x%x\nas->heap_end:0x%x\nas->as_regions->permissions:%d\nvaddr:0x%x\npaddr:0x%x\nallocated:%d\nnum_pages:%d\n", as->heap_start, as->heap_end, as->as_regions->next->permissions, as->as_regions->next->vaddr, as->as_regions->next->paddr, as->as_regions->next->allocated, as->as_regions->next->num_pages);

	as->as_stackpbase = getppages(SMARTVM_STACKPAGES);
	if (as->as_stackpbase == 0) {
		return ENOMEM;
	}

	return 0;

}

int
as_complete_load(struct addrspace *as)
{
	/*
	 * Write this.
	 */

	//kprintf("as_complete_load\n");

	(void)as;
	return 0;
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
	/*
	 * Write this.
	 */

	assert(as->as_stackpbase != 0);

	*stackptr = USERSTACK;
	return 0;
}

