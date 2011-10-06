#ifndef _OS_INTERFACE_H
#define _OS_INTERFACE_H

#include <unistd.h>


//new system call
#define __NR_absys_mmap			337
#define __NR_absys_thread_control	338

//ab_thread_control_flag
#define AB_SET_ME_SPECIAL	0x00000000
#define AB_SET_ME_ARBITER	0x00000001

/*
system call arbilloc will take (input + 1) as output
*/

static inline int absys_mmap(int aaa)
{
	return syscall(__NR_absys_mmap, aaa);
}

static inline int absys_thread_control(int ab_thread_control_flag)
{
	return syscall(__NR_absys_thread_control, ab_thread_control_flag);
}
#endif //_OS_INTERFACE_H
