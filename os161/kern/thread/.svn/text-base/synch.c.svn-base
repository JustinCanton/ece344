/*
 * Synchronization primitives.
 * See synch.h for specifications of the functions.
 */

#include <types.h>
#include <lib.h>
#include <synch.h>
#include <thread.h>
#include <curthread.h>
#include <machine/spl.h>

////////////////////////////////////////////////////////////
//
// Semaphore.

struct semaphore *
sem_create(const char *namearg, int initial_count)
{
	struct semaphore *sem;

	assert(initial_count >= 0);

	sem = kmalloc(sizeof(struct semaphore));
	if (sem == NULL) {
		return NULL;
	}

	sem->name = kstrdup(namearg);
	if (sem->name == NULL) {
		kfree(sem);
		return NULL;
	}

	sem->count = initial_count;
	return sem;
}

void
sem_destroy(struct semaphore *sem)
{
	int spl;
	assert(sem != NULL);

	spl = splhigh();
	assert(thread_hassleepers(sem)==0);
	splx(spl);

	/*
	 * Note: while someone could theoretically start sleeping on
	 * the semaphore after the above test but before we free it,
	 * if they're going to do that, they can just as easily wait
	 * a bit and start sleeping on the semaphore after it's been
	 * freed. Consequently, there's not a whole lot of point in 
	 * including the kfrees in the splhigh block, so we don't.
	 */

	kfree(sem->name);
	kfree(sem);
}

void 
P(struct semaphore *sem)
{
	int spl;
	assert(sem != NULL);

	/*
	 * May not block in an interrupt handler.
	 *
	 * For robustness, always check, even if we can actually
	 * complete the P without blocking.
	 */
	assert(in_interrupt==0);

	spl = splhigh();
	while (sem->count==0) {
		thread_sleep(sem);
	}
	assert(sem->count>0);
	sem->count--;
	splx(spl);
}

void
V(struct semaphore *sem)
{
	int spl;
	assert(sem != NULL);
	spl = splhigh();
	sem->count++;
	assert(sem->count>0);
	thread_wakeup(sem);
	splx(spl);
}

////////////////////////////////////////////////////////////
//
// Lock.

struct lock *
lock_create(const char *name)
{
	struct lock *lock;

	lock = kmalloc(sizeof(struct lock));
	if (lock == NULL) {
		return NULL;
	}

	lock->name = kstrdup(name);
	if (lock->name == NULL) {
		kfree(lock);
		return NULL;
	}
	
	lock->flag = 0;
	lock->currentThread = NULL;
	
	return lock;
}

void
lock_destroy(struct lock *lock)
{
	assert(lock != NULL);
	
	kfree(lock->name);
	kfree(lock);
}

void
lock_acquire(struct lock *lock)
{
	int spl;	// Declare a spl variable to manipulate the interrupt handler
	spl = splhigh();		// Disable interrupts
	
	assert(lock != NULL);	// Make sure the lock isn't NULL
	assert(in_interrupt == 0);	// Make sure we aren't in an interrupt handler

	while(lock->flag != 0){		// Check to see if the lock is in use
		thread_sleep(lock);
	}

	lock->flag = 1;		// Give the lock to the thread
	lock->currentThread = curthread;		// Get the thread that has the lock
	
	splx(spl);		// Enable interrupts
}

void
lock_release(struct lock *lock)
{
	int spl;	// Declare a spl variable to manipulate the interrupt handler
	spl = splhigh();		// Disable interrupts
	
	assert(lock != NULL);	// Make sure the lock isn't NULL
	assert(in_interrupt == 0);	// Make sure we aren't in an interrupt handler

	if(lock_do_i_hold(lock) == 0)
		return;

	lock->flag = 0;		// Make the thread not have the lock anymore
	lock->currentThread = NULL;		// Make the current thread null
	thread_wakeup(lock);

	splx(spl);		// Enable interrupts
}

int
lock_do_i_hold(struct lock *lock)
{
	assert(lock != NULL);	// Make sure the lock isn't NULL

	if(lock->currentThread == curthread)		// If the current thread is the same as the thread holding the lock
		return 1;		// Return true
	return 0;		// Otherwise return false
}

////////////////////////////////////////////////////////////
//
// CV


struct cv *
cv_create(const char *name)
{
	struct cv *cv;

	cv = kmalloc(sizeof(struct cv));
	if (cv == NULL) {
		return NULL;
	}

	cv->name = kstrdup(name);
	if (cv->name==NULL) {
		kfree(cv);
		return NULL;
	}
	
	return cv;
}

void
cv_destroy(struct cv *cv)
{
	assert(cv != NULL);
	
	kfree(cv->name);
	kfree(cv);
}

void
cv_wait(struct cv *cv, struct lock *lock)
{
	int spl;	// Declare a spl variable to manipulate the interrupt handler
	spl = splhigh();		// Disable interrupts

	assert(cv != NULL);		// Make sure the cv isn't NULL
	assert(lock != NULL);		// Make sure the lock isn't NULL
	assert(in_interrupt == 0);	// Make sure we aren't in an interrupt handler

	lock_release(lock);		// Release the lock held by the thread
	thread_sleep(cv);		// Set the thread to sleep on the cv
	lock_acquire(lock);		// After waking up, reaquire the lock

	splx(spl);		// Enable interrupts
}

void
cv_signal(struct cv *cv, struct lock *lock)
{
	int spl;	// Declare a spl variable to manipulate the interrupt handler
	spl = splhigh();		// Disable interrupts

	assert(cv != NULL);		// Make sure the cv isn't NULL
	assert(lock != NULL);		// Make sure the lock isn't NULL
	assert(in_interrupt == 0);	// Make sure we aren't in an interrupt handler

	thread_single_wakeup(cv);		// Wake up one of the threads on the cv

	splx(spl);		// Enable interrupts
}

void
cv_broadcast(struct cv *cv, struct lock *lock)
{
	int spl;	// Declare a spl variable to manipulate the interrupt handler
	spl = splhigh();		// SDisable interrupts

	assert(cv != NULL);		// Make sure the cv isn't NULL
	assert(lock != NULL);		// Make sure the lock isn't NULL
	assert(in_interrupt == 0);	// Make sure we aren't in an interrupt handler

	thread_wakeup(cv);			// Wake up all the threads on the cv

	splx(spl);		// Enable interrupts
}
