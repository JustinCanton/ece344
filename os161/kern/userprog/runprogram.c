/*
 * Sample/test code for running a user program.  You can use this for
 * reference when implementing the execv() system call. Remember though
 * that execv() needs to do more than this function does.
 */

#include <types.h>
#include <kern/unistd.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <thread.h>
#include <curthread.h>
#include <vm.h>
#include <vfs.h>
#include <test.h>

/*
 * Load program "progname" and start running it in usermode.
 * Does not return except on error.
 *
 * Calls vfs_open on progname and thus may destroy it.
 */
int
runprogram(char *progname, char **argv, int nargs)
{
	struct vnode *v;
	vaddr_t entrypoint, stackptr;
	int result;
	int u_argc = 0;
	int calcu_length, offset;
	userptr_t new_args[nargs];

	/* Open the file. */
	result = vfs_open(progname, O_RDONLY, &v);
	if (result) {
		return result;
	}

	/* We should be a new thread. */
	assert(curthread->t_vmspace == NULL);

	/* Create a new address space. */
	curthread->t_vmspace = as_create();
	if (curthread->t_vmspace==NULL) {
		vfs_close(v);
		return ENOMEM;
	}

	/* Activate it. */
	as_activate(curthread->t_vmspace);

	/* Load the executable. */
	result = load_elf(v, &entrypoint);
	if (result) {
		/* thread_exit destroys curthread->t_vmspace */
		vfs_close(v);
		return result;
	}

	/* Done with the file now. */
	vfs_close(v);

	/* Define the user stack in the address space */
	result = as_define_stack(curthread->t_vmspace, &stackptr);
	if (result) {
		/* thread_exit destroys curthread->t_vmspace */
		return result;
	}

	//Send the kernel arguments to the stack
	while(u_argc < nargs){
		calcu_length = strlen(argv[u_argc]) + 1;

		//Move the stack pointer the necessary amount to account for the arguments
		stackptr -= calcu_length;

		//align the stack
		if(stackptr % 4 != 0){
			stackptr -= stackptr % 4;
		}

		//Add a NULL terminator at the end of the argument
		argv[u_argc][calcu_length - 1] = '\0';
		
		//Copy out the kernel arguments
		result = copyout(argv[u_argc], (userptr_t) stackptr, calcu_length);
		
		switch(result){
			case 0: 
			break;
			
			default: 
			return result;
			break;
		}

		//set the new arguments to the stack pointer
		new_args[u_argc] = (userptr_t) stackptr;
		u_argc++;
	}

	new_args[u_argc] = 0;

	//Realign the stack 
	offset = sizeof(userptr_t) * (nargs + 1);
	stackptr -= offset;
	stackptr -= stackptr % 8;

	//Copy out the new arguments
	result = copyout(new_args, (userptr_t) stackptr, offset);

	switch(result){
		case 0: 
		break;
		
		default: 
		return result;
		break;
	}

	/* Warp to user mode. */
	md_usermode(nargs, (userptr_t) stackptr, stackptr, entrypoint);
	
	/* md_usermode does not return */
	panic("md_usermode returned\n");
	return EINVAL;
}

