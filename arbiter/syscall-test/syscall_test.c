#include <sys/syscall.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>

#include <ab_os_interface.h>

int main()
{ 
	int aaa = 5;
	pid_t pid;

	/* test absys_mmap() */
	printf("if aaa = %d\n", aaa);
	printf("absys_mmap(aaa) =  %d\n", absys_mmap(aaa));

	/* test absys_thread_control */
	if (absys_thread_control(AB_SET_ME_ARBITER)==0)
		printf("parent thread setting complete!\n");
	
	pid = fork();
	if (pid == 0) {		/* child "thread" */
  		if (absys_thread_control(AB_SET_ME_COMMON)==0)
			printf("child thread setting complete!\n");
	}
  	if (pid > 0) {		/* parent "thread" */
		wait(NULL);
		printf("child thread complete!\n");
		exit(0);
	}
//	return 0;
}
