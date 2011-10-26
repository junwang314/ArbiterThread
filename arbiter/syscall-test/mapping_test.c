#include <sys/syscall.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>

#include <ab_os_interface.h>


//number of child thread
#define NUM_THREADS 20


void child_func()
{
	pause();
	printf("doing some work...\n");
	sleep(1);
	printf("work done\n");
}


extern void mapping_test()
{
	int i, status;
	pid_t pid[NUM_THREADS], wpid;
	for (i = 0; i < NUM_THREADS; ++i) {
		pid[i] = fork();
		if (pid[i] < 0) {
			perror("thread cannot be created.\n");
			exit(0);
		}
		if(pid[i] == 0) {
			printf("child process %i created, pid = %lu.\n", i, (unsigned long)getpid());
			child_func();
			exit(0);
		}		
	}
	//wait some time for all the child to start
	sleep(6);

	//wake up child
	for (i = 0; i < NUM_THREADS; ++i) {
		kill(pid[i], SIGCONT);
	}

	while(1) {
		wpid = waitpid(-1, &status, 0);
		if(WIFEXITED(status)) {
			printf("child %lu exited normally.\n", wpid);
		}
		else
			printf("wait pid returns with status %d\n", status);
	}
	
				  
}
