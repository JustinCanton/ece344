#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <machine/pcb.h>
#include <machine/spl.h>
#include <machine/trapframe.h>
#include <kern/callno.h>
#include <syscall.h>
#include <uio.h>
#include <vnode.h>
#include <thread.h>
#include <curthread.h>
#include <synch.h>
#include <vfs.h>
#include <addrspace.h>
#include <kern/unistd.h>
#include <vm.h>
#include <test.h>

/*
 * This is the system call that prints to user input  
 */
int sys_print(int fd, userptr_t *buf, size_t nbytes, int32_t *retval){

	//declare variables and structures
	struct uio printer;
	struct vnode *vn;
	int error;
	char *console = NULL;

	//check errors
	if(!fd){
		*retval = -1;
		return EBADF;
	}

	//open up console
	console = kstrdup("con:");
	error = vfs_open(console, 0, &vn);

	//check errors
	switch(error){
		case 0: 
		break;
		
		default: 
		*retval = -1;
		return error;
		break;
	}

	//create a uio that writes
	mk_uuio(&printer, buf, nbytes, 0, UIO_WRITE);

	kfree(console);

	//WRITE to the user console
	error = VOP_WRITE(vn, &printer);

	//check errors
	switch(error){
		case 0: 
		*retval = nbytes;
		return 0;
		break;
		
		default: 
		*retval = -1;
		return error;
		break;
	}
}

/*
 * This is the system call that reads from user input  
 */
int sys_read(int fd, userptr_t *buf, size_t buflen, int32_t *retval){

	//declare variables and structures
	struct uio reader;
	struct vnode *vn;
	int error;
	char *console = NULL;

	//check errors
	if(fd){
		*retval = -1;
		return EBADF;
	}

	if(buf == NULL){
		*retval = -1;
		return EFAULT;
	}

	//open up console
	console = kstrdup("con:");
	error = vfs_open(console, 0, &vn);

	//check errors
	switch(error){
		case 0: 
		break;
		
		default: 
		*retval = -1;
		return error;
		break;
	}

	//create a uio that reads
	mk_uuio(&reader, buf, buflen, 0, UIO_READ);

	kfree(console);

	//READ from the user console
	error = VOP_READ(vn, &reader);

	//check errors
	switch(error){
		case 0: 
		*retval = buflen;
		return 0;
		break;
		
		default: 
		*retval = -1;
		return error;
		break;
	}
}

/*
 * This is the system call returns the pid of the current thread
 */
int sys_getpid(pid_t *retval){
	
	*retval = curthread->myPid;
	return 0;
}

/*
 * This is the system call that creates a new process
 */
int sys_fork(struct trapframe *tf, int32_t *retval){
	
	//declare structures and variables
	struct thread* newgirl;
    struct addrspace* newgirl_address_space; 
    struct trapframe* newgirl_trapframe = (struct trapframe*)kmalloc(sizeof(struct trapframe));
    int error;

    //copy the address space of the parent to the child
	error = as_copy(curthread->t_vmspace, &newgirl_address_space);

	//check the errors	
	switch(error){
		case 0: 
		break;
		
		default: 
		*retval = -1;
		return error;
		break;
	}

	//make sure that newgirl_trapframe was allocated
    if (newgirl_trapframe == NULL){
    	*retval = -1;
        return  ENOMEM;
	}

	//copy the parents trapframe into the child trapframe
	memcpy(newgirl_trapframe, tf, sizeof(struct trapframe));

	//create a new child thread
	error = thread_fork("new_thread", newgirl_trapframe, (unsigned long)newgirl_address_space, child_fork, &newgirl);

	//check the errors
	switch(error){
		case 0: 
		*retval = newgirl->myPid;
		return 0;
		break;
		
		default: 
		*retval = -1;
		return error;
		break;
	}
}

/*
 * This is the return function for the child when thread_fork is called
 */
void child_fork(struct trapframe *tf, unsigned long address_space){

	//change the values of the trapframe for the child process
	tf->tf_v0 = 0;
	tf->tf_a3 = 0;
	tf->tf_epc += 4;

	//activate the address space
	curthread->t_vmspace = (struct addrspace *) address_space;
	as_activate(curthread->t_vmspace);

	//give the child a new trapframe
	struct trapframe return_tf = *tf;

	//run the child process
	mips_usermode(&return_tf);
}

/*
 * This system call makes the current thread wait for t_pid to exit
 */
int sys_waitpid(pid_t t_pid, int *status, int options, pid_t *retval){

	//create variables
	int wait_status, new_ret, error;

	//Check errors
	if (t_pid <= 0 || t_pid > MAX_PIDS){
		*retval = -1;
		return EINVAL;
	}
	if (curthread->myPid == t_pid){
		*retval = -1;
		return EINVAL;
	}
	if (status == NULL){
		*retval=-1;
		return EFAULT;
	}
	if (options != 0){
		*retval=-1;
		return EINVAL;
	}
	
	//call function pid_wait
	error = pid_wait(t_pid, &wait_status, &new_ret);

	//set status to the exit code and retval to pid
	*status = wait_status;
	*retval = new_ret;

	return error;
}

/*
 * This system call exits the thread and kills it
 */
int sys__exit(int exitcode){

	//call our pid_exit function
	pid_exit(exitcode);
	thread_exit();
	return 0;
}


/*
 * This system call takes in arguments that the user passes 
 * and creates a new process with those arguments
 */
int sys_execv(const char *program, char **args, int32_t *retval){

	//Structures and variables we declared
	struct vnode *v; 
	vaddr_t entrypoint, stackptr;
	int result, argc, actual_length, offset, calcu_length;
	int u_argc = 0;
	int random_number = 1024;
	char **karg = (char **)kmalloc(sizeof(char));
	char destination[random_number + 1];
	char kernel_arg[random_number + 1];

	//copy the program name
	result = copyinstr(program, destination, random_number, &actual_length);

	//If error break out and return
	switch(result){
		case 0: 
		break;
		
		default: 
		*retval = -1;
		return result;
		break;
	}

	//if there is no destination break out and return
	switch(strlen(destination)){
		case 0:
		*retval = -1;
		return ENOENT;	
		break;
		
		default:
		break;
	}

	//add a NULL terminator at the end of the destination
	destination[actual_length -1] = '\0';

	for(argc = 0; args[argc] != NULL; argc++){
		
		if(argc > 4){
			*retval = -1;
			return E2BIG;
		}
		
		//copy in each of the arguments from the user level to the kernel level
		result = copyinstr(args[argc], kernel_arg, random_number, &actual_length);
		
		switch(result){
			case 0: 
			break;
			
			default: 
			*retval = -1;
			return result;
			break;
		}

		//add a NULL terminator at the end of the argument and store it in the kernel args
		kernel_arg[actual_length - 1] = '\0';
		karg[argc] = kernel_arg;
	}
	
	//create a new user argument array
	userptr_t new_args[argc];

	/* Open the file. */
	result = vfs_open(destination, O_RDONLY, &v);
	
	switch(result){
		case 0: 
		break;
		
		default: 
		*retval = -1;
		return result;
		break;
	}

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
	while(u_argc < argc){
		calcu_length = strlen(karg[u_argc]) + 1;

		//Move the stack pointer the necessary amount to account for the arguments
		stackptr -= calcu_length;

		//align the stack
		if(stackptr % 4 != 0){
			stackptr -= stackptr % 4;
		}

		//Add a NULL terminator at the end of the argument
		karg[u_argc][calcu_length - 1] = '\0';
		
		//Copy out the kernel arguments
		result = copyoutstr(karg[u_argc], (userptr_t) stackptr, calcu_length, NULL);
		
		switch(result){
			case 0: 
			break;
			
			default: 
			*retval = -1;
			return result;
			break;
		}

		//set the new arguments to the stack pointer
		new_args[u_argc] = (userptr_t) stackptr;
		u_argc++;
		kfree(karg);
	}

	new_args[u_argc] = 0;

	//Realign the stack 
	offset = sizeof(userptr_t) * (argc + 1);
	stackptr -= offset;
	stackptr -= stackptr % 8;

	//Copy out the new arguments
	result = copyout(new_args, (userptr_t) stackptr, offset);

	switch(result){
		case 0: 
		break;
		
		default: 
		*retval = -1;
		return result;
		break;
	}

	/* Warp to user mode. */
	md_usermode(argc, (userptr_t) stackptr, stackptr, entrypoint);
	
	/* md_usermode does not return */
	panic("md_usermode returned\n");
	return EINVAL;
}

/*
 * This system call moves the end address of the heap region, 
 * then returns the old nd of the heap
 */
int sys_sbrk(intptr_t amount, int32_t *retval){

	int pages, i;
	struct pte *temp;
	struct pte *make;
	struct pte *release;
	struct pte *holder;

	if(amount == 0){
		*retval = curthread->t_vmspace->heap_end;
		return 0;
	}

	/*if(amount % 4){
		amount += 4 - (amount % 4);
	}*/

	//kprintf("1: heap_start: 0x%x, heap_end: 0x%x, amount: %d\n", curthread->t_vmspace->heap_start, curthread->t_vmspace->heap_end, amount);

	if(amount < 0){
		//kprintf("amount: %d", amount);
		if((curthread->t_vmspace->heap_end + amount) < curthread->t_vmspace->heap_start){
			//kprintf("in break");
			*retval = -1;
			return EINVAL;
		}

		amount *= -1;
		
		if(amount < PAGE_SIZE){
			*retval = curthread->t_vmspace->heap_end;
			curthread->t_vmspace->heap_end -= amount;
			return 0;
		}else {
			pages = amount/PAGE_SIZE;
			if(amount % PAGE_SIZE){
				pages++;
			}

			for(i=0; i < pages; i++){

				for(temp = curthread->t_vmspace->heap; temp->next->next != NULL; temp = temp->next);
				free_kpages(temp->next->paddr);
				kfree(temp->next);
				temp->next = NULL;
			}

			//kprintf("heap_start: 0x%x, heap_end: 0x%x\n", curthread->t_vmspace->heap_start, curthread->t_vmspace->heap_end);
			*retval = curthread->t_vmspace->heap_end;
			curthread->t_vmspace->heap_end -= amount;
		}
	}else{
		//kprintf("2: heap_start: 0x%x, heap_end: 0x%x, amount: %d\n", curthread->t_vmspace->heap_start, curthread->t_vmspace->heap_end, amount);

		if(amount < PAGE_SIZE && ((((curthread->t_vmspace->heap_end - curthread->t_vmspace->heap_start) % PAGE_SIZE) + amount) < PAGE_SIZE)){
			if(curthread->t_vmspace->heap == NULL){
				//kprintf("Making new\n");
				make = (struct pte *)kmalloc(sizeof(struct pte));
				make->paddr = getppages(1);
				make->vaddr = PADDR_TO_KVADDR(make->paddr);
				make->next = NULL;
				curthread->t_vmspace->heap = make;
			}
			*retval = curthread->t_vmspace->heap_end;
			curthread->t_vmspace->heap_end += amount;
			return 0;
		}else{
			pages = amount/PAGE_SIZE;
			if(amount % PAGE_SIZE){
				pages++;
			}

			if(curthread->t_vmspace->heap == NULL){
				make = (struct pte *)kmalloc(sizeof(struct pte));
				make->paddr = getppages(1);
				make->vaddr = PADDR_TO_KVADDR(make->paddr);
				make->next = NULL;
				curthread->t_vmspace->heap = make;
				temp = curthread->t_vmspace->heap;
				release = temp;
				for(i = 0; i < (pages - 1); i++){
					make = (struct pte *)kmalloc(sizeof(struct pte));
					make->paddr = getppages(1);
					//kprintf("paddr: 0x%x, i: %d\n", make->paddr, i);
					if(make->paddr == 0){
						if(release->next == NULL){
							free_kpages(release->vaddr);
							kfree(release);
							kfree(make);
							curthread->t_vmspace->heap = NULL;
							*retval = -1;
							return ENOMEM;
						}
						release = release->next;
						while(release->next != NULL){
							temp = release;
							release = release->next;
							//kprintf("beginning: releasing paddr: 0x%x, temp paddr: 0x%x\n", release->paddr, temp->paddr);
							free_kpages(temp->vaddr);
							kfree(temp);
						}
						free_kpages(release->vaddr);
						kfree(release);
						kfree(make);
						curthread->t_vmspace->heap = NULL;
						*retval = -1;
						return ENOMEM;
					}
					make->vaddr = PADDR_TO_KVADDR(make->paddr);
					make->next = NULL;
					temp->next = make;
					temp = temp->next;
				}
			}else{
				for(temp = curthread->t_vmspace->heap; temp->next != NULL; temp = temp->next);

				release = temp;
				holder = release;

				for(i = 0; i < (pages - 1); i++){
					make = (struct pte *)kmalloc(sizeof(struct pte));
					make->paddr = getppages(1);
					//kprintf("paddr: 0x%x, i: %d\n", make->paddr, i);
					if(make->paddr == 0){
						if(release->next == NULL){
							kfree(make);
							*retval = -1;
							return ENOMEM;
						}
						release = release->next;
						while(release->next != NULL){
							temp = release;
							release = release->next;
							//kprintf("from point: releasing paddr: 0x%x, temp paddr: 0x%x\n", release->paddr, temp->paddr);
							free_kpages(temp->vaddr);
							kfree(temp);
						}
						free_kpages(release->vaddr);
						kfree(release);
						kfree(make);
						holder->next = NULL;
						*retval = -1;
						return ENOMEM;
					}
					make->vaddr = PADDR_TO_KVADDR(make->paddr);
					make->next = NULL;
					temp->next = make;
					temp = temp->next;
				}
			}
		}
		//kprintf("heap_end: 0x%x", curthread->t_vmspace->heap_end);
		*retval = curthread->t_vmspace->heap_end;
		curthread->t_vmspace->heap_end += amount;
	}

	return 0;
}
