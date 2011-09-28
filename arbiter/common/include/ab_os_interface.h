#ifndef _OS_INTERFACE_H
#define _OS_INTERFACE_H

#include <unistd.h>


//new system call
#define __NR_arbilloc	337

/*
system call arbilloc will take (input + 1) as output
*/

static inline int arbilloc(int aaa)
{
	return syscall(__NR_arbilloc, aaa);
}


#endif //_OS_INTERFACE_H
