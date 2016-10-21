/*
 * forktest - test fork().
 *
 * This should work correctly when fork is implemented.
 *
 * It should also continue to work after subsequent assignments, most
 * notably after implementing the virtual memory system.
 */

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <err.h>

/*
 * This is used by all processes, to try to help make sure all
 * processes have a distinct address space.
 */
static volatile int mypid;
static volatile int yourpid;
/*
 * Helper function for fork that prints a warning on error.
 */

int
main(int argc, char *argv[])
{
	int x;
	mypid = fork();
	printf("The mypid is %d\n", mypid);
	yourpid = fork();
	printf("The yourpid is %d\n", yourpid);
	waitpid(yourpid, &x, 0);
	waitpid(mypid, &x, 0);
	return 0;
}
