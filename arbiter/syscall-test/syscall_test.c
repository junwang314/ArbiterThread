#include <sys/syscall.h>
#include <unistd.h>
#include <stdio.h>



#include <ab_os_interface.h>

int main()
{ 
	int aaa = 5;
	printf("if aaa = %d\n", aaa);
	printf("absys_mmap(aaa) =  %d\n", absys_mmap(aaa));
  	if (absys_fork()==0)
		printf("absys_fork works!\n");
	return 0;
}
