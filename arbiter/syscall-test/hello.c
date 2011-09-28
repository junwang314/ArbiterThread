#include <sys/syscall.h>
#include <unistd.h>
#include <stdio.h>

#define __NR_arbilloc	337

/*
system call arbilloc will take (input + 1) as output
*/

int arbilloc(int aaa)
{
	return syscall(__NR_arbilloc,aaa);
}

int main()
{ 
	int aaa = 5;
	printf("if aaa = %d\n", aaa);
	printf("arbilloc(aaa) =  %d\n", arbilloc(aaa));
  	return 0;
}
