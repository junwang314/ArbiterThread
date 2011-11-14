#ifndef _OS_INTERFACE_H
#define _OS_INTERFACE_H

#include <unistd.h>

#include <sys/mman.h>
#include <sys/types.h>


//new system call
#define __NR_absys_mmap			337
#define __NR_absys_thread_control	338
#define __NR_absys_brk			339
#define __NR_absys_munmap		340

//ab_thread_control_flag
#define AB_SET_ME_SPECIAL	0x00000000
#define AB_SET_ME_ARBITER	0x00000001



/* called by the arbiter thread, pid designates a child thread in its */
/* control group, however the call would affect virtual address
allocation of all the child threads in its control group */

static inline void *absys_mmap (pid_t pid, void *addr, size_t len, int prot,
		   int flags, int fd, __off_t offset)
{
	return (void *) syscall(__NR_absys_mmap, pid, addr, len, prot, 
		       flags, fd, offset);
}


static inline int absys_thread_control(int ab_thread_control_flag)
{
	return syscall(__NR_absys_thread_control, ab_thread_control_flag);
}

static inline unsigned long absys_brk(pid_t pid, void *addr)
{
	unsigned long ab_brk = (unsigned long)addr;
	return syscall(__NR_absys_brk, pid, ab_brk);
	//return (syscall(__NR_absys_brk, pid, brk) == brk) ? 0 : -1;
}

static inline int absys_munmap(pid_t pid, void *addr, size_t length)
{
	return syscall(__NR_absys_munmap, pid, addr, length);
}
#endif //_OS_INTERFACE_H
