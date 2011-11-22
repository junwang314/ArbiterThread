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
#define __NR_absys_mprotect		341


//ab_thread_control_flag
#define AB_SET_ME_SPECIAL	0x00000000
#define AB_SET_ME_ARBITER	0x00000001



/* called by the arbiter thread, pid designates a child thread in its
 * control group, however the call would affect virtual address
 * allocation of all the child threads in its control group:
 * 1) For the target child thread designated by pid:
 *    the interval [addr, addr+len] is mapped as what mmap() does;
 * 2) For the other childs in the control group:
 *    the minimum interference principle will be applied, which means
 *    only map the holes between existing VMAs.
 */
static inline void *absys_mmap (pid_t pid, void *addr, size_t len, int prot,
		   int flags, int fd, __off_t offset)
{
	return (void *) syscall(__NR_absys_mmap, pid, addr, len, prot, 
		       flags, fd, offset);
}

/* called by any theads in the control group to set themselves as
 * either the arbiter thread or other child threads
 */
static inline int absys_thread_control(int ab_thread_control_flag)
{
	return syscall(__NR_absys_thread_control, ab_thread_control_flag);
}

/* called by the arbiter thread. 
 * !!! only need to call ONCE even though a child pid needs to be provided !!!
 * synchronously set the end point (i.e. ab_brk) of ab-heap.
 * fails if any kinds of VMA (sys VMA and abt VMA) stand in the way.
 */
static inline unsigned long absys_brk(pid_t pid, void *addr)
{
	unsigned long ab_brk = (unsigned long)addr;
	return syscall(__NR_absys_brk, pid, ab_brk);
	//return (syscall(__NR_absys_brk, pid, brk) == brk) ? 0 : -1;
}

/* called by the arbiter thread, pid designates a child thread in its
 * control group, however the call would affect virtual address
 * allocation of all the child threads in its control group:
 * 1) For the target child thread designated by pid:
 *    the interval [addr, addr+len] is unmapped as what munmmap() does;
 * 2) For the other childs in the control group:
 *    the minimum interference principle will be applied, which means
 *    only abt VMAs are unmapped.
 */
static inline int absys_munmap(pid_t pid, void *addr, size_t length)
{
	return syscall(__NR_absys_munmap, pid, addr, length);
}


static inline int absys_mprotect(pid_t pid, void *addr, size_t len, int prot)
{
	return syscall(__NR_absys_mprotect, pid, addr, len, prot);
}

#endif //_OS_INTERFACE_H
