/* 
 * stoplight.c
 *
 * 31-1-2003 : GWA : Stub functions created for CS161 Asst1.
 *
 * NB: You can use any synchronization primitives available to solve
 * the stoplight problem in this file.
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
 * Number of cars created.
 */

#define NCARS 20


/*
 *
 * Function Definitions
 *
 */

static const char *directions[] = { "N", "E", "S", "W" };

static const char *msgs[] = {
        "approaching:",
        "region1:    ",
        "region2:    ",
        "region3:    ",
        "leaving:    "
};

/* use these constants for the first parameter of message */
enum { APPROACHING, REGION1, REGION2, REGION3, LEAVING };

/*
 *
 * Glocal Variables
 *
 */
static struct semaphore *north_west;        // The north west semaphore
static struct semaphore *south_west;        // The south west semaphore
static struct semaphore *north_east;        // The north east semaphore
static struct semaphore *south_east;        // The south east semaphore
static int north = 1;       // The current number that is at the north intersection
static int south = 1;       // The current number that is at the south intersection
static int east = 1;       // The current number that is at the east intersection
static int west = 1;       // The current number that is at the west intersection
static int north_assign = 0;        // The order which the cars appear at the north intersection
static int south_assign = 0;        // The order which the cars appear at the south intersection
static int east_assign = 0;     // The order which the cars appear at the east intersection
static int west_assign = 0;     // The order which the cars appear at the west intersection

static void
message(int msg_nr, int carnumber, int cardirection, int destdirection)
{
        kprintf("%s car = %2d, direction = %s, destination = %s\n",
                msgs[msg_nr], carnumber,
                directions[cardirection], directions[destdirection]);
}
 
/*
 * gostraight()
 *
 * Arguments:
 *      unsigned long cardirection: the direction from which the car
 *              approaches the intersection.
 *      unsigned long carnumber: the car id number for printing purposes.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      This function should implement passing straight through the
 *      intersection from any direction.
 *      Write and comment this function.
 */

static
void
gostraight(unsigned long cardirection,
           unsigned long carnumber,
          int order)
{
        /*
         * Avoid unused variable warnings.
         */
        
        (void) cardirection;
        (void) carnumber;

        switch(cardirection){       //cardirection is the where the car is coming from 
            case 0:                 //if cardirection is 0 (North) 
                while(north != order); //this is here so that cars do not pass each other, we are keeping the order
                P(north_west);  // Acquire the North West block semaphore
                P(south_west);  // Acquire the South West block semaphore
                message(1, carnumber, cardirection, 2); // Region 1 message 
                message(2, carnumber, cardirection, 2); // Region 2 message 
                V(north_west);                          // Release the North West semaphore
                message(4, carnumber, cardirection, 2); // Leaving Intersection message 
                V(south_west);                          // Release the South West semaphore
                north++;                                // Increment the car number in the north lane
                break;
            case 1:                 //if cardirection is 1 (East) 
                while(east != order);
                P(north_west);  // Acquire the North West block semaphore
                P(north_east);  // Acquire the North East block semaphore
                message(1, carnumber, cardirection, 3); // Region 1 message 
                message(2, carnumber, cardirection, 3); // Region 2 message 
                V(north_east);                          // Release the North East semaphore
                message(4, carnumber, cardirection, 3); // Leaving Intersection message 
                V(north_west);                          // Release the North West semaphore
                east++;                                 // Increment the car number in the east lane
                break;
            case 2:                 //if cardirection is 2 (South) 
                while(south != order);
                P(north_east);  // Acquire the North East block semaphore
                P(south_east);  // Acquire the South East block semaphore
                message(1, carnumber, cardirection, 0); // Region 1 message 
                message(2, carnumber, cardirection, 0); // Region 2 message 
                V(south_east);                          // Release the South East semaphore
                message(4, carnumber, cardirection, 0); // Leaving Intersection message 
                V(north_east);                          // Release the North East semaphore
                south++;                                // Increment the car number in the south lane
                break;
            case 3:                 //if cardirection is 3 (West) 
                while(west != order);
                P(south_west);  // Acquire the South West block semaphore
                P(south_east);  // Acquire the South East block semaphore
                message(1, carnumber, cardirection, 1); // Region 1 message 
                message(2, carnumber, cardirection, 1); // Region 2 message 
                V(south_west);                          // Release the South West semaphore
                message(4, carnumber, cardirection, 1); // Leaving Intersection message 
                V(south_east);                          // Release the South East semaphore
                west++;                                 // Increment the car number in the south lane
                break;
        }
}


/*
 * turnleft()
 *
 * Arguments:
 *      unsigned long cardirection: the direction from which the car
 *              approaches the intersection.
 *      unsigned long carnumber: the car id number for printing purposes.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      This function should implement making a left turn through the 
 *      intersection from any direction.
 *      Write and comment this function.
 */

static
void
turnleft(unsigned long cardirection,
         unsigned long carnumber,
          int order)
{
        /*
         * Avoid unused variable warnings.
         */

        (void) cardirection;
        (void) carnumber;

        switch(cardirection){       //cardirection is the where the car is coming from 
            case 0:                 //if cardirection is 0 (North) 
                while(north != order);  //this is here so that cars do not pass each other, we are keeping the order
                P(north_west);  // Acquire the North West block semaphore
                P(south_west);  // Acquire the South West block semaphore
                P(south_east);  // Acquire the South East block semaphore
                message(1, carnumber, cardirection, 1); // Region 1 message 
                message(2, carnumber, cardirection, 1); // Region 2 message 
                V(north_west);                          // Release the North West semaphore
                message(3, carnumber, cardirection, 1); // Region 3 message
                V(south_west);                          // Release the South West semaphore
                message(4, carnumber, cardirection, 1); // Leaving Intersection message
                V(south_east);                          // Release the South East semaphore
                north++;                                // Increment the car number in the north lane
                break;
            case 1:                 //if cardirection is 1 (East) 
                while(east != order);//this is here so that cars do not pass each other, we are keeping the order
                P(north_west);  // Acquire the North West block semaphore
                P(south_west);  // Acquire the South West block semaphore
                P(north_east);  // Acquire the North East block semaphore
                message(1, carnumber, cardirection, 2);  // Region 1 message 
                message(2, carnumber, cardirection, 2);  // Region 2 message
                V(north_east);                           // Release the North East semaphore
                message(3, carnumber, cardirection, 2);  // Region 3 message
                V(north_west);                           // Release the North West semaphore
                message(4, carnumber, cardirection, 2);  // Leaving Intersection message
                V(south_west);                           // Release the South West semaphore
                east++;                                  // Increment the car number in the east lane
                break;
            case 2:                 //if cardirection is 2 (South) 
                while(south != order);//this is here so that cars do not pass each other, we are keeping the order
                P(north_west); // Acquire the North West block semaphore
                P(north_east); // Acquire the North East block semaphore
                P(south_east); // Acquire the South East block semaphore
                message(1, carnumber, cardirection, 3);   // Region 1 message 
                message(2, carnumber, cardirection, 3);   // Region 2 message
                V(south_east);                            // Release the South East semaphore
                message(3, carnumber, cardirection, 3);   // Region 3 message
                V(north_east);                            // Release the North East semaphore
                message(4, carnumber, cardirection, 3);   // Leaving Intersection message
                V(north_west);                            // Release the North West semaphore
                south++;                                  // Increment the car number in the south lane
                break;
            case 3:                 //if cardirection is 3 (West) 
                while(west != order);//this is here so that cars do not pass each other, we are keeping the order
                P(south_west);  // Acquire the South West block semaphore
                P(north_east);  // Acquire the North East block semaphore
                P(south_east);  // Acquire the South East block semaphore
                message(1, carnumber, cardirection, 0);   // Region 1 message 
                message(2, carnumber, cardirection, 0);   // Region 2 message
                V(south_west);                            // Release the South West semaphore
                message(3, carnumber, cardirection, 0);   // Region 3 message
                V(south_east);                            // Release the South East semaphore
                message(4, carnumber, cardirection, 0);   // Leaving Intersection message
                V(north_east);                            // Release the North East semaphore
                west++;                                   // Increment the car number in the east lane
                break;
        }
}


/*
 * turnright()
 *
 * Arguments:
 *      unsigned long cardirection: the direction from which the car
 *              approaches the intersection.
 *      unsigned long carnumber: the car id number for printing purposes.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      This function should implement making a right turn through the 
 *      intersection from any direction.
 *      Write and comment this function.
 */

static
void
turnright(unsigned long cardirection,
          unsigned long carnumber,
          int order)
{
        /*
         * Avoid unused variable warnings.
         */

        (void) cardirection;
        (void) carnumber;

        switch(cardirection){       //cardirection is the where the car is coming from 
            case 0:                 //if cardirection is 0 (North) 
                while(north != order);//this is here so that cars do not pass each other, we are keeping the order
                P(north_west);  // Acquire the North West block semaphore
                message(1, carnumber, cardirection, 3);   // Region 1 message
                message(4, carnumber, cardirection, 3);   // Leaving Intersection message
                V(north_west);                            // Release the North West semaphore
                north++;                                  // Increment the car number in the north lane
                break;
            case 1:                 //if cardirection is 1 (East) 
                while(east != order);
                P(north_east);  // Acquire the North East block semaphore
                message(1, carnumber, cardirection, 0);   // Region 1 message
                message(4, carnumber, cardirection, 0);   // Leaving Intersection message
                V(north_east);                            // Release the North East semaphore
                east++;                                   // Increment the car number in the east lane
                break;
            case 2:                 //if cardirection is 2 (South) 
                while(south != order);
                P(south_east);  // Acquire the North East block semaphore
                message(1, carnumber, cardirection, 1);   // Region 1 message
                message(4, carnumber, cardirection, 1);   // Leaving Intersection message
                V(south_east);                            // Release the South East semaphore
                south++;                                  // Increment the car number in the south lane
                break;
            case 3:                 //if cardirection is 3 (West) 
                while(west != order);
                P(south_west);  // Acquire the South West block semaphore
                message(1, carnumber, cardirection, 2);   // Region 1 message
                message(4, carnumber, cardirection, 2);   // Leaving Intersection message
                V(south_west);                            // Release the South West semaphore
                west++;                                   // Increment the car number in the west lane
                break;
        }
}


/*
 * approachintersection()
 *
 * Arguments: 
 *      void * unusedpointer: currently unused.
 *      unsigned long carnumber: holds car id number.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Change this function as necessary to implement your solution. These
 *      threads are created by createcars().  Each one must choose a direction
 *      randomly, approach the intersection, choose a turn randomly, and then
 *      complete that turn.  The code to choose a direction randomly is
 *      provided, the rest is left to you to implement.  Making a turn
 *      or going straight should be done by calling one of the functions
 *      above.
 */
 
static
void
approachintersection(void * unusedpointer,
                     unsigned long carnumber)
{
        int cardirection, destdirection;

        /*
         * Avoid unused variable and function warnings.
         */

        (void) unusedpointer;
        (void) carnumber;
        (void) gostraight;
        (void) turnleft;
        (void) turnright;

        /*
         * cardirection is set randomly.
         */

        cardirection = random() % 4;

        /*
         * destdirection is set randomly.
         */

        while((destdirection = random() % 4) == cardirection);

        message(0, carnumber, cardirection, destdirection);     // Message that the car is approaching the intersection

        if(cardirection == 0){      // If the car is approaching from the north
            switch(destdirection){
                case 1:         // If the car is going to the east
                    north_assign++;     // Increment the counter for the number of cars coming from the north
                    turnleft(cardirection, carnumber, north_assign);        // Turn left
                    break;
                case 2:         // If the car is going to the south
                    north_assign++;     // Increment the counter for the number of cars coming from the north
                    gostraight(cardirection, carnumber, north_assign);      // Go straight
                    break;
                case 3:         // If the car is going west
                    north_assign++;     // Increment the counter for the number of cars coming from the north
                    turnright(cardirection, carnumber, north_assign);       // Turn right
                    break;
            }
        }else if(cardirection == 1){        // If the car is approaching from the east
            switch(destdirection){
                case 0:         // If the car is going to the north
                    east_assign++;     // Increment the counter for the number of cars coming from the east
                    turnright(cardirection, carnumber, east_assign);        // Turn right
                    break;
                case 2:         // If the car is going south
                    east_assign++;     // Increment the counter for the number of cars coming from the east
                    turnleft(cardirection, carnumber, east_assign);     // Turn left
                    break;
                case 3:         // If the car is going west
                    east_assign++;     // Increment the counter for the number of cars coming from the east
                    gostraight(cardirection, carnumber, east_assign);       // Go straight
                    break;
            }
        }else if(cardirection == 2){        // If the car is approaching from the south
            switch(destdirection){
                case 0:         // If the car is going north
                    south_assign++;     // Increment the counter for the number of cars coming from the south
                    gostraight(cardirection, carnumber, south_assign);      // Go straight
                    break;
                case 1:         // If the car is going east
                    south_assign++;     // Increment the counter for the number of cars coming from the south
                    turnright(cardirection, carnumber, south_assign);       // Turn right
                    break;
                case 3:         // If the car is going west
                    south_assign++;     // Increment the counter for the number of cars coming from the south
                    turnleft(cardirection, carnumber, south_assign);        // Turn left
                    break;
            }
        }else if(cardirection == 3){        // If the car is approaching from the west
            switch(destdirection){
                case 0:         // If the car is going north
                    west_assign++;     // Increment the counter for the number of cars coming from the west
                    turnleft(cardirection, carnumber, west_assign);     // Turn left
                    break;
                case 1:         // If the car is going east
                    west_assign++;     // Increment the counter for the number of cars coming from the west
                    gostraight(cardirection, carnumber, west_assign);       // Go straight
                    break;
                case 2:         // If the car is going south
                    west_assign++;     // Increment the counter for the number of cars coming from the west
                    turnright(cardirection, carnumber, west_assign);        // Turn right
                    break;
            }
        }
}


/*
 * createcars()
 *
 * Arguments:
 *      int nargs: unused.
 *      char ** args: unused.
 *
 * Returns:
 *      0 on success.
 *
 * Notes:
 *      Driver code to start up the approachintersection() threads.  You are
 *      free to modiy this code as necessary for your solution.
 */

int
createcars(int nargs,
           char ** args)
{
        int index, error;

        /*
         * Avoid unused variable warnings.
         */

        (void) nargs;
        (void) args;

        if (north_west == NULL) {       // Create the north west semaphore
            north_west = sem_create("north_west", 1);
            if (north_west == NULL) {
                panic("stoplight: sem_create failed\n");
            }
        }
        if (south_west == NULL) {       // Create the south west semaphore
            south_west = sem_create("south_west", 1);
            if (south_west == NULL) {
                panic("stoplight: sem_create failed\n");
            }
        }
        if (north_east == NULL) {       // Create the north east semaphore
            north_east = sem_create("north_east", 1);
            if (north_east == NULL) {
                panic("stoplight: sem_create failed\n");
            }
        }
        if (south_east == NULL) {       // Create the south east semaphore
            south_east = sem_create("south_east", 1);
            if (south_east == NULL) {
                panic("stoplight: sem_create failed\n");
            }
        }

        /*
         * Start NCARS approachintersection() threads.
         */

        for (index = 0; index < NCARS; index++) {

                error = thread_fork("approachintersection thread",
                                    NULL,
                                    index,
                                    approachintersection,
                                    NULL
                                    );

                /*
                 * panic() on error.
                 */

                if (error) {
                        
                        panic("approachintersection: thread_fork failed: %s\n",
                              strerror(error)
                              );
                }
        }

        return 0;
}
