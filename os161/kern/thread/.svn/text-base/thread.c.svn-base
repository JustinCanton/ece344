/*
 * Core thread system.
 */
#include <types.h>
#include <lib.h>
#include <kern/errno.h>
#include <array.h>
#include <machine/spl.h>
#include <machine/pcb.h>
#include <thread.h>
#include <curthread.h>
#include <scheduler.h>
#include <addrspace.h>
#include <vnode.h>
#include <synch.h>
#include "opt-synchprobs.h"


/* States a thread can be in. */
typedef enum {
	S_RUN,
	S_READY,
	S_SLEEP,
	S_ZOMB,
} threadstate_t;

/* Global variable for the thread currently executing at any given time. */
struct thread *curthread;

/* Table of sleeping threads. */
static struct array *sleepers;

/* List of dead threads to be disposed of. */
static struct array *zombies;

/* Total number of outstanding threads. Does not count zombies[]. */
static int numthreads;

/* The structure containing the pid's */
static struct process *pid[MAX_PIDS];  

/* The structure containing the pid's */
static struct lock *pid_lock;  

/*
 * Create a thread. This is used both to create the first thread's 
 * thread structure and to create subsequent threads.
 */

static
struct thread *
thread_create(const char *name)
{
	struct thread *thread = kmalloc(sizeof(struct thread));
	if (thread==NULL) {
		return NULL;
	}
	thread->t_name = kstrdup(name);
	if (thread->t_name==NULL) {
		kfree(thread);
		return NULL;
	}
	thread->t_sleepaddr = NULL;
	thread->t_stack = NULL;
	
	thread->t_vmspace = NULL;

	thread->t_cwd = NULL;

	thread->myPid = 0;

	return thread;
}

/*
 * Destroy a thread.
 *
 * This function cannot be called in the victim thread's own context.
 * Freeing the stack you're actually using to run would be... inadvisable.
 */
static
void
thread_destroy(struct thread *thread)
{
	assert(thread != curthread);

	// If you add things to the thread structure, be sure to dispose of
	// them here or in thread_exit.

	// These things are cleaned up in thread_exit.
	assert(thread->t_vmspace==NULL);
	assert(thread->t_cwd==NULL);
	
	if (thread->t_stack) {
		kfree(thread->t_stack);
	}

	kfree(thread->t_name);
	kfree(thread);
}


/*
 * Remove zombies. (Zombies are threads/processes that have exited but not
 * been fully deleted yet.)
 */
static
void
exorcise(void)
{
	int i, result;

	assert(curspl>0);
	
	for (i=0; i<array_getnum(zombies); i++) {
		struct thread *z = array_getguy(zombies, i);
		assert(z!=curthread);
		thread_destroy(z);
	}
	result = array_setsize(zombies, 0);
	/* Shrinking the array; not supposed to be able to fail. */
	assert(result==0);
}

/*
 * Kill all sleeping threads. This is used during panic shutdown to make 
 * sure they don't wake up again and interfere with the panic.
 */
static
void
thread_killall(void)
{
	int i, result;

	assert(curspl>0);

	/*
	 * Move all sleepers to the zombie list, to be sure they don't
	 * wake up while we're shutting down.
	 */

	for (i=0; i<array_getnum(sleepers); i++) {
		struct thread *t = array_getguy(sleepers, i);
		kprintf("sleep: Dropping thread %s\n", t->t_name);

		/*
		 * Don't do this: because these threads haven't
		 * been through thread_exit, thread_destroy will
		 * get upset. Just drop the threads on the floor,
		 * which is safer anyway during panic.
		 *
		 * array_add(zombies, t);
		 */
	}

	result = array_setsize(sleepers, 0);
	/* shrinking array: not supposed to fail */
	assert(result==0);
}

/*
 * Shut down the other threads in the thread system when a panic occurs.
 */
void
thread_panic(void)
{
	assert(curspl > 0);

	thread_killall();
	scheduler_killall();
}

/*
 * Thread initialization.
 */
struct thread *
thread_bootstrap(void)
{
	struct thread *me;

	/* Create the data structures we need. */
	sleepers = array_create();
	if (sleepers==NULL) {
		panic("Cannot create sleepers array\n");
	}

	zombies = array_create();
	if (zombies==NULL) {
		panic("Cannot create zombies array\n");
	}
	
	/*
	 * Create the thread structure for the first thread
	 * (the one that's already running)
	 */
	me = thread_create("<boot/menu>");
	if (me==NULL) {
		panic("thread_bootstrap: Out of memory\n");
	}

	/*
	 * Leave me->t_stack NULL. This means we're using the boot stack,
	 * which can't be freed.
	 */

	/* Initialize the first thread's pcb */
	md_initpcb0(&me->t_pcb);

	/* Set curthread */
	curthread = me;

	/* Number of threads starts at 1 */
	numthreads = 1;

	/* Done */
	return me;
}

/*
 * Thread final cleanup.
 */
void
thread_shutdown(void)
{
	array_destroy(sleepers);
	sleepers = NULL;
	array_destroy(zombies);
	zombies = NULL;
	// Don't do this - it frees our stack and we blow up
	//thread_destroy(curthread);
}

/*
 * Create a new thread based on an existing one.
 * The new thread has name NAME, and starts executing in function FUNC.
 * DATA1 and DATA2 are passed to FUNC.
 */
int
thread_fork(const char *name, 
	    void *data1, unsigned long data2,
	    void (*func)(void *, unsigned long),
	    struct thread **ret)
{
	struct thread *newguy;
	int s, result;

	/* Allocate a thread */
	newguy = thread_create(name);
	if (newguy==NULL) {
		return ENOMEM;
	}

	/* Allocate a stack */
	newguy->t_stack = kmalloc(STACK_SIZE);
	if (newguy->t_stack==NULL) {
		kfree(newguy->t_name);
		kfree(newguy);
		return ENOMEM;
	}

	/* stick a magic number on the bottom end of the stack */
	newguy->t_stack[0] = 0xae;
	newguy->t_stack[1] = 0x11;
	newguy->t_stack[2] = 0xda;
	newguy->t_stack[3] = 0x33;

	/* Inherit the current directory */
	if (curthread->t_cwd != NULL) {
		VOP_INCREF(curthread->t_cwd);
		newguy->t_cwd = curthread->t_cwd;
	}

	/* Set up the pcb (this arranges for func to be called) */
	md_initpcb(&newguy->t_pcb, newguy->t_stack, data1, data2, func);

	/* Interrupts off for atomicity */
	s = splhigh();

	/*
	 * Make sure our data structures have enough space, so we won't
	 * run out later at an inconvenient time.
	 */
	result = array_preallocate(sleepers, numthreads+1);
	if (result) {
		goto fail;
	}
	result = array_preallocate(zombies, numthreads+1);
	if (result) {
		goto fail;
	}

	/* Do the same for the scheduler. */
	result = scheduler_preallocate(numthreads+1);
	if (result) {
		goto fail;
	}

	/* Make the new thread runnable */
	result = make_runnable(newguy);
	if (result != 0) {
		goto fail;
	}

	//allocate a new pid for the child thread
	lock_acquire(pid_lock);
	newguy->myPid = pid_insert(curthread->myPid, &result);
	lock_release(pid_lock);

	/*
	 * Increment the thread counter. This must be done atomically
	 * with the preallocate calls; otherwise the count can be
	 * temporarily too low, which would obviate its reason for
	 * existence.
	 */
	numthreads++;

	/* Done with stuff that needs to be atomic */
	splx(s);

	/*
	 * Return new thread structure if it's wanted.  Note that
	 * using the thread structure from the parent thread should be
	 * done only with caution, because in general the child thread
	 * might exit at any time.
	 */
	if (ret != NULL) {
		*ret = newguy;
	}

	return 0;

 fail:
	splx(s);
	if (newguy->t_cwd != NULL) {
		VOP_DECREF(newguy->t_cwd);
	}
	kfree(newguy->t_stack);
	kfree(newguy->t_name);
	kfree(newguy);

	return result;
}

/*
 * High level, machine-independent context switch code.
 */
static
void
mi_switch(threadstate_t nextstate)
{
	struct thread *cur, *next;
	int result;
	
	/* Interrupts should already be off. */
	assert(curspl>0);

	if (curthread != NULL && curthread->t_stack != NULL) {
		/*
		 * Check the magic number we put on the bottom end of
		 * the stack in thread_fork. If these assertions go
		 * off, it most likely means you overflowed your stack
		 * at some point, which can cause all kinds of
		 * mysterious other things to happen.
		 */
		assert(curthread->t_stack[0] == (char)0xae);
		assert(curthread->t_stack[1] == (char)0x11);
		assert(curthread->t_stack[2] == (char)0xda);
		assert(curthread->t_stack[3] == (char)0x33);
	}
	
	/* 
	 * We set curthread to NULL while the scheduler is running, to
	 * make sure we don't call it recursively (this could happen
	 * otherwise, if we get a timer interrupt in the idle loop.)
	 */
	if (curthread == NULL) {
		return;
	}
	cur = curthread;
	curthread = NULL;

	/*
	 * Stash the current thread on whatever list it's supposed to go on.
	 * Because we preallocate during thread_fork, this should not fail.
	 */

	if (nextstate==S_READY) {
		result = make_runnable(cur);
	}
	else if (nextstate==S_SLEEP) {
		/*
		 * Because we preallocate sleepers[] during thread_fork,
		 * this should never fail.
		 */
		result = array_add(sleepers, cur);
	}
	else {
		assert(nextstate==S_ZOMB);
		result = array_add(zombies, cur);
	}
	assert(result==0);

	/*
	 * Call the scheduler (must come *after* the array_adds)
	 */

	next = scheduler();

	/* update curthread */
	curthread = next;
	
	/* 
	 * Call the machine-dependent code that actually does the
	 * context switch.
	 */
	md_switch(&cur->t_pcb, &next->t_pcb);
	
	/*
	 * If we switch to a new thread, we don't come here, so anything
	 * done here must be in mi_threadstart() as well, or be skippable,
	 * or not apply to new threads.
	 *
	 * exorcise is skippable; as_activate is done in mi_threadstart.
	 */

	exorcise();

	if (curthread->t_vmspace) {
		as_activate(curthread->t_vmspace);
	}
}

/*
 * Cause the current thread to exit.
 *
 * We clean up the parts of the thread structure we don't actually
 * need to run right away. The rest has to wait until thread_destroy
 * gets called from exorcise().
 */
void
thread_exit(void)
{
	if (curthread->t_stack != NULL) {
		/*
		 * Check the magic number we put on the bottom end of
		 * the stack in thread_fork. If these assertions go
		 * off, it most likely means you overflowed your stack
		 * at some point, which can cause all kinds of
		 * mysterious other things to happen.
		 */
		assert(curthread->t_stack[0] == (char)0xae);
		assert(curthread->t_stack[1] == (char)0x11);
		assert(curthread->t_stack[2] == (char)0xda);
		assert(curthread->t_stack[3] == (char)0x33);
	}

	splhigh();

	if (curthread->t_vmspace) {
		/*
		 * Do this carefully to avoid race condition with
		 * context switch code.
		 */
		struct addrspace *as = curthread->t_vmspace;
		curthread->t_vmspace = NULL;
		as_destroy(as);
	}

	if (curthread->t_cwd) {
		VOP_DECREF(curthread->t_cwd);
		curthread->t_cwd = NULL;
	}

	assert(numthreads>0);
	numthreads--;
	mi_switch(S_ZOMB);

	panic("Thread came back from the dead!\n");
}

/*
 * Yield the cpu to another process, but stay runnable.
 */
void
thread_yield(void)
{
	int spl = splhigh();

	/* Check sleepers just in case we get here after shutdown */
	assert(sleepers != NULL);

	mi_switch(S_READY);
	splx(spl);
}

/*
 * Yield the cpu to another process, and go to sleep, on "sleep
 * address" ADDR. Subsequent calls to thread_wakeup with the same
 * value of ADDR will make the thread runnable again. The address is
 * not interpreted. Typically it's the address of a synchronization
 * primitive or data structure.
 *
 * Note that (1) interrupts must be off (if they aren't, you can
 * end up sleeping forever), and (2) you cannot sleep in an 
 * interrupt handler.
 */
void
thread_sleep(const void *addr)
{
	// may not sleep in an interrupt handler
	assert(in_interrupt==0);
	
	curthread->t_sleepaddr = addr;
	mi_switch(S_SLEEP);
	curthread->t_sleepaddr = NULL;
}

/*
 * Wake up one or more threads who are sleeping on "sleep address"
 * ADDR.
 */
void
thread_wakeup(const void *addr)
{
	int i, result;
	
	// meant to be called with interrupts off
	assert(curspl>0);
	
	// This is inefficient. Feel free to improve it.
	
	for (i=0; i<array_getnum(sleepers); i++) {
		struct thread *t = array_getguy(sleepers, i);
		if (t->t_sleepaddr == addr) {
			
			// Remove from list
			array_remove(sleepers, i);
			
			// must look at the same sleepers[i] again
			i--;

			/*
			 * Because we preallocate during thread_fork,
			 * this should never fail.
			 */
			result = make_runnable(t);
			assert(result==0);
		}
	}
}

/*
 * Wake up one thread that is sleeping on "sleep address"
 * ADDR.
 * A copy of thread_wakeup with no for loop
 */
void
thread_single_wakeup(const void *addr)
{
	int result;
	
	// meant to be called with interrupts off
	assert(curspl>0);
	
	// This is inefficient. Feel free to improve it.
	struct thread *t = array_getguy(sleepers, 0);
	if (t->t_sleepaddr == addr) {
		
		// Remove from list
		array_remove(sleepers, 0);
		
		// must look at the same sleepers[i] again

		/*
		 * Because we preallocate during thread_fork,
		 * this should never fail.
		 */
		result = make_runnable(t);
		assert(result==0);
	}
}

/*
 * Return nonzero if there are any threads who are sleeping on "sleep address"
 * ADDR. This is meant to be used only for diagnostic purposes.
 */
int
thread_hassleepers(const void *addr)
{
	int i;
	
	// meant to be called with interrupts off
	assert(curspl>0);
	
	for (i=0; i<array_getnum(sleepers); i++) {
		struct thread *t = array_getguy(sleepers, i);
		if (t->t_sleepaddr == addr) {
			return 1;
		}
	}
	return 0;
}

/*
 * New threads actually come through here on the way to the function
 * they're supposed to start in. This is so when that function exits,
 * thread_exit() can be called automatically.
 */
void
mi_threadstart(void *data1, unsigned long data2, 
	       void (*func)(void *, unsigned long))
{
	/* If we have an address space, activate it */
	if (curthread->t_vmspace) {
		as_activate(curthread->t_vmspace);
	}

	/* Enable interrupts */
	spl0();

#if OPT_SYNCHPROBS
	/* Yield a random number of times to get a good mix of threads */
	{
		int i, n;
		n = random()%161 + random()%161;
		for (i=0; i<n; i++) {
			thread_yield();
		}
	}
#endif
	
	/* Call the function */
	func(data1, data2);

	/* Done. */
	thread_exit();
}

/*
 * This function is used to allocate the main OS thread and create the pid lock  
 */
void pid_bootstrap(void){
	
	//allocate the first pid for the main OS thread
	pid[0] = pid_alloc(0, NULL);

	//set the lock to NULL
	pid_lock = NULL;
	
	//create the pid lock 
	if (pid_lock == NULL) {
		pid_lock = lock_create("pid_lock");
		if (pid_lock == NULL) {
			panic("pid: lock_create failed\n");
		}
	}
}

/*
 * This function is used to allocate threads  
 */
struct process *pid_alloc(pid_t pid, pid_t p_pid){
	
	//Declare variables
	struct process *newpid;

	//Allocate space for the process
	newpid = kmalloc(sizeof(struct process));
	
	//set the values of the process using pid and p_pid
	newpid->thread_pid = pid;
	newpid->parent_pid = p_pid;
	newpid->exitcode = 0;
	newpid->exited = 0;
	newpid->has_waiters = 0;
	newpid->exit_semaphore = NULL;

	//create the exit sempahore
	if (newpid->exit_semaphore == NULL) {
		newpid->exit_semaphore = sem_create("exit_semaphore", 0);
		if (newpid->exit_semaphore == NULL) {
			panic("pid: sem_create failed\n");
		}
	}

	//return the newly allocated process
	return newpid;
}

/*
 * This function is used to deallocate threads  
 */
void pid_dealloc(pid_t t_pid){
	
	//destory the semaphores, free the allocated space and set the current pid to NULL
	sem_destroy(pid[t_pid]->exit_semaphore);
	kfree(pid[t_pid]);
	pid[t_pid] = NULL;
}

/*
 * This function is used to return an allocated pid  
 */
int pid_insert(pid_t parent, int32_t *retval){

	//Declare variable
	int i;

	//look throught the pid array to find the next available slot for the new pid
	for(i = 0; i < MAX_PIDS; i++){
		if(pid[i] == NULL){
			break;
		}
	}

	//check for errors
	if(i == MAX_PIDS){
		*retval = -1;
		return EAGAIN;
	}

	//call pid allocate to get the pid
	pid[i] = pid_alloc(i, parent);

	return i;
}

/*
 * This function is used to exit  
 */
void pid_exit(unsigned exitcode){
	
	//declare variables
	int i;

	//look throught the pid array to find the spot of the current pid
	for(i = 0; i < MAX_PIDS; i++){
		if(pid[i] == NULL){

		}
		else if(pid[i]->thread_pid == curthread->myPid){
			break;
		}
	}

	//check if the current thread has others waiting on it if so deallocate
	switch(pid[i]->has_waiters){
		case 0:
		pid_dealloc(i);
		break;
		
		//set the exit code to 1 and decrement the semaphore
		default:
		pid[i]->exitcode = exitcode;
		pid[i]->exited = 1;
		V(pid[i]->exit_semaphore);
		break;
	}
}

/*
 * This function is used to wait and see if the pid has exited   
 */
int pid_wait(pid_t t_pid, int *status, int *retval){

	//if the pid doesnt exist then return the pid because it has exited 
	if(pid[t_pid] == NULL){
		*status = 0;
		*retval = t_pid;
		return 0;
	}

	//Check errors
	if(curthread->myPid != pid[t_pid]->parent_pid){
		*retval=-1;
		return EINVAL;
	}

	//This pid has some thread waiting for it to exit
	pid[t_pid]->has_waiters++;

	//decrement the semaphore
	P(pid[t_pid]->exit_semaphore);

	*status = pid[t_pid]->exitcode;
	*retval = t_pid;
	
	pid[t_pid]->has_waiters--;

	//if pid has anyone waiting on it then dellocate otherwise increment the exit semaphore
	switch(pid[t_pid]->has_waiters){
		case 0:
		pid_dealloc(t_pid);
		break;
		
		default:
		V(pid[t_pid]->exit_semaphore);
		break;
	}

	return 0;
}
