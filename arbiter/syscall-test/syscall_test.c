#include <sys/syscall.h>
#include <unistd.h>
#include <stdio.h>



#include <ab_os_interface.h>

int main()
{ 
	int aaa = 5;
	printf("if aaa = %d\n", aaa);
	printf("arbilloc(aaa) =  %d\n", arbilloc(aaa));
  	return 0;
}
