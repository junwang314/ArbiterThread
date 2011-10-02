#ifndef _OS_INTERFACE_H
#define _OS_INTERFACE_H

#include <unistd.h>


//new system call
#define __NR_absys_mmap		337
#define __NR_absys_fork		338

/*
system call arbilloc will take (input + 1) as output
*/

static inline int absys_mmap(int aaa)
{
	return syscall(__NR_absys_mmap, aaa);
}

static inline int absys_fork(void)
{
	return syscall(__NR_absys_fork);
}
#endif //_OS_INTERFACE_H
