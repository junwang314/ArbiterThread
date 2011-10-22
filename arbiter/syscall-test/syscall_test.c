#include <sys/syscall.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>

#include <stdlib.h>

#include <ab_os_interface.h>

int main()
{ 
	int aaa = 5;
	pid_t pid1, pid2, pid3;

	/* test absys_mmap() */
	//printf("if aaa = %d\n", aaa);
	printf("absys_mmap =  %p\n", absys_mmap(1234, (void *) 0xdeadbeef,
						     4096, 7, 8, 9, 128));


	/* test absys_thread_control */
	if (absys_thread_control(AB_SET_ME_ARBITER)==0)
		printf("parent thread setting complete!\n");
	
	pid1 = fork();
	if (pid1 == 0) {	/* child "thread" 1 */
	  	if (absys_thread_control(AB_SET_ME_SPECIAL)==0)
			printf("child thread 1 setting complete!\n");
	  	pid2 = fork();
		if (pid2 == 0) {	/* child "thread" 2 */
			if (absys_thread_control(AB_SET_ME_SPECIAL)==0)
				printf("child thread 2 setting complete!\n");
			pid3 = fork();
			if (pid3 == 0) {	/* child "thread" 3 */
				if (absys_thread_control(AB_SET_ME_SPECIAL)==0)
					printf("child thread 3 setting complete!\n");
			}
			if (pid3 > 0 ) {	/* parent "thread" */
				wait(NULL);
				printf("child thread 3 complete!\n");
				exit(0);
			}		
		}
		if (pid2 > 0 ) {	/* parent "thread" */
			wait(NULL);
			printf("child thread 2 complete!\n");
			exit(0);
		}		
	}
  	if (pid1 > 0) {		/* parent "thread" */
		wait(NULL);
		printf("child thread 1 complete!\n");
		exit(0);
	}
//	return 0;
}
