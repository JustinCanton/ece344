#ifndef _SYSCALL_H_
#define _SYSCALL_H_

/*
 * Prototypes for IN-KERNEL entry points for system call implementations.
 */

int sys_reboot(int code);
int sys_print(int fd, userptr_t *buf, size_t nbytes, int32_t *retval);
int sys_read(int fd, userptr_t *buf, size_t buflen, int32_t *retval);
int sys_fork(struct trapframe *tf, int32_t *retval);
void child_fork(struct trapframe *tf, unsigned long address_space);
int sys_getpid(pid_t *retval);
int sys_waitpid(pid_t t_pid, int *status, int options, pid_t *retval);
int sys__exit(int exitcode);
int sys_execv(const char *program, char **args, int32_t *retval);
int sys_sbrk(intptr_t amount, int32_t *retval);


#endif /* _SYSCALL_H_ */
