/*
 * catlock.c
 *
 * 30-1-2003 : GWA : Stub functions created for CS161 Asst1.
 *
 * NB: Please use LOCKS/CV'S to solve the cat syncronization problem in 
 * this file.
 */


/*
 * 
 * Includes
 *
 */

#include <types.h>
#include <lib.h>
#include <test.h>
#include <thread.h>
#include <synch.h>


/*
 * 
 * Constants
 *
 */

/*
 * Number of food bowls.
 */

#define NFOODBOWLS 2

/*
 * Number of cats.
 */

#define NCATS 6

/*
 * Number of mice.
 */

#define NMICE 2

/*
 * Global Variables
 */
static struct lock *bowl_1;     // Lock for the first bowl
static struct lock *bowl_2;     // Lock for the second bowl
static int cat_eating = 0;      // The cat eating varaible
static int mouse_eating = 0;        // The mouse eating variable
static struct cv *cat;      // The cat cv
static struct cv *mouse;        // The mouse cv
static struct lock *sleep_mouse;        // The sleep mouse lock
static struct lock *sleep_cat;      // The sleep cat lock
static struct lock *printing_lock;      // The sleep cat lock

/*
 * 
 * Function Definitions
 * 
 */

/* who should be "cat" or "mouse" */
static void
lock_eat(const char *who, int num, int bowl, int iteration)
{
        lock_acquire(printing_lock);
        kprintf("%s: %d starts eating: bowl %d, iteration %d\n", who, num, 
                bowl, iteration);
        lock_release(printing_lock);
        clocksleep(1);
        lock_acquire(printing_lock);
        kprintf("%s: %d ends eating: bowl %d, iteration %d\n", who, num, 
                bowl, iteration);
        lock_release(printing_lock);
}

/*
 * catlock()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long catnumber: holds the cat identifier from 0 to NCATS -
 *      1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Write and comment this function using locks/cv's.
 *
 */

static
void
catlock(void * unusedpointer, 
        unsigned long catnumber)
{
        /*
         * Avoid unused variable warnings.
         */

        (void) unusedpointer;
        (void) catnumber;

        int iteration;      // Keeps track of the number of times a cat has eaten

        for(iteration = 0; iteration < 4; iteration++){     // Loop 4 times
            while(mouse_eating > 0){        // While the mice are eating
                cv_wait(cat, sleep_cat);        // Wait and sleep
            }

            cat_eating++;       // Increment the cat eating to know how many cats are eating

            if(bowl_1->flag == 0 && mouse_eating == 0){     // If bowl 1 isn't occupied and no mice are eating
                lock_acquire(bowl_1);       // Acquire the 1st bowl lock
                lock_eat("cat", catnumber, 1, iteration);       // Call the lock eat for this thread
                lock_release(bowl_1);       // Release the lock that this thread has on bowl 1
                lock_release(sleep_cat);        // Release the lock on the sleep cat
                cat_eating--;       // Decrement the cat eating variable
                if(cat_eating == 0){        // If there are no cats eating
                    cv_broadcast(mouse, sleep_mouse);       // Wake up the mice and get them eating
                }
            }else if(lock_do_i_hold(bowl_2) == 0 && mouse_eating == 0){     // If bowl 2 isn't occupied and no mice are eating
                lock_acquire(bowl_2);       // Acquire the 2nd bowl lock
                lock_eat("cat", catnumber, 2, iteration);       // Call the lock eat for this thread
                lock_release(bowl_2);       // Release the lock that this thread has on bowl 2
                lock_release(sleep_cat);        // Release the lock on the sleep cat
                cat_eating--;       // Decrement the cat eating variable
                if(cat_eating == 0){        // If there are no cats eating
                    cv_broadcast(mouse, sleep_mouse);       // Wake up the mice and get them eating
                }
            }else{      // If there are no locks open and it skips both if statements
                iteration--;        // Flip back the iteration and go around again
                cat_eating--;       // Decrement the cat eating variable, since it isn't eating
                lock_release(sleep_cat);        // Release the lock on the sleep cat
            }
        }
}
    

/*
 * mouselock()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long mousenumber: holds the mouse identifier from 0 to 
 *              NMICE - 1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Write and comment this function using locks/cv's.
 *
 */

static
void
mouselock(void * unusedpointer,
          unsigned long mousenumber)
{
        /*
         * Avoid unused variable warnings.
         */
        
        (void) unusedpointer;
        (void) mousenumber;

        int iteration;      // Keeps track of the number of times a cat has eaten

        for(iteration = 0; iteration < 4; iteration++){     // Loop 4 times
            while(cat_eating > 0){      // While the cats are eating
                cv_wait(mouse, sleep_mouse);        // Wait and sleep
            }

            mouse_eating++;     // Increment the mouse eating to know how many mice are eating

            if(bowl_1->flag == 0 && cat_eating == 0){       // If bowl 1 isn't occupied and no cats are eating
                lock_acquire(bowl_1);       // Acquire the 1st bowl lock
                lock_eat("mouse", mousenumber, 1, iteration);       // Call the lock eat for this thread
                lock_release(bowl_1);       // Release the lock that this thread has on bowl 1
                lock_release(sleep_mouse);      // Release the lock on the sleep mouse
                mouse_eating--;     // Decrement the mouse eating variable
                if(mouse_eating == 0){      // If there are no mice eating
                    cv_broadcast(cat, sleep_cat);       // Wake up the cats and get them eating
                }
            }else if(lock_do_i_hold(bowl_2) == 0 && cat_eating == 0){       // If bowl 2 isn't occupied and no cats are eating
                lock_acquire(bowl_2);       // Acquire the 2nd bowl lock
                lock_eat("mouse", mousenumber, 2, iteration);       // Call the lock eat for this thread
                lock_release(bowl_2);       // Release the lock that this thread has on bowl 2
                lock_release(sleep_mouse);      // Release the lock on the sleep mouse
                mouse_eating--;         // Decrement the mouse eating variable
                if(mouse_eating == 0){      // If there are no mice eating
                    cv_broadcast(cat, sleep_cat);       // Wake up the cats and get them eating
                }
            }else{      // If there are no locks open and it skips both if statements
                iteration--;        // Flip back the iteration and go around again
                mouse_eating--;     // Decrement the mouse eating variable, since it isn't eating
                lock_release(sleep_mouse);       // Release the lock on the sleep mouse
            }
        }
}


/*
 * catmouselock()
 *
 * Arguments:
 *      int nargs: unused.
 *      char ** args: unused.
 *
 * Returns:
 *      0 on success.
 *
 * Notes:
 *      Driver code to start up catlock() and mouselock() threads.  Change
 *      this code as necessary for your solution.
 */

int
catmouselock(int nargs,
             char ** args)
{
        int index, error;
   
        /*
         * Avoid unused variable warnings.
         */

        (void) nargs;
        (void) args;
        
        /*
         * Initialize the locks and cvs.
         */

        if (bowl_1 == NULL) {       // Create the bowl 1 lock
            bowl_1 = lock_create("bowl_1");
            if (bowl_1 == NULL) {
                panic("catlock: lock_create failed\n");
            }
        }
        if (bowl_2 == NULL) {       // Create the bowl 2 lock
            bowl_2 = lock_create("bowl_2");
            if (bowl_2 == NULL) {
                panic("catlock: lock_create failed\n");
            }
        }
        if (cat == NULL) {      // Create the cat cv
            cat = cv_create("cat");
            if (cat == NULL) {
                panic("catlock: cv_create failed\n");
            }
        }
        if (mouse == NULL) {        // Create the mouse cv
            mouse = cv_create("mouse");
            if (mouse == NULL) {
                panic("catlock: cv_create failed\n");
            }
        }
        if (sleep_mouse == NULL) {      // Create the sleep mouse lock
            sleep_mouse = lock_create("sleep_mouse");
            if (sleep_mouse == NULL) {
                panic("catlock: lock_create failed\n");
            }
        }
        if (sleep_cat == NULL) {        // Create the sleep cat lock
            sleep_cat = lock_create("sleep_cat");
            if (sleep_cat == NULL) {
                panic("catlock: lock_create failed\n");
            }
        }
        if (printing_lock == NULL) {        // Create the sleep cat lock
            printing_lock = lock_create("printing_lock");
            if (printing_lock == NULL) {
                panic("catlock: lock_create failed\n");
            }
        }

        /*
         * Start NCATS catlock() threads.
         */

        for (index = 0; index < NCATS; index++) {
           
                error = thread_fork("catlock thread", 
                                    NULL, 
                                    index, 
                                    catlock, 
                                    NULL
                                    );
                
                /*
                 * panic() on error.
                 */

                if (error) {
                 
                        panic("catlock: thread_fork failed: %s\n", 
                              strerror(error)
                              );
                }
        }

        /*
         * Start NMICE mouselock() threads.
         */

        for (index = 0; index < NMICE; index++) {
   
                error = thread_fork("mouselock thread", 
                                    NULL, 
                                    index, 
                                    mouselock, 
                                    NULL
                                    );
      
                /*
                 * panic() on error.
                 */

                if (error) {
         
                        panic("mouselock: thread_fork failed: %s\n", 
                              strerror(error)
                              );
                }
        }

        return 0;
}

/*
 * End of catlock.c
 */
