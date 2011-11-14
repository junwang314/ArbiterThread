#include <sys/syscall.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>

#include <ab_os_interface.h>


//number of child thread
#define NUM_THREADS 5


static void suicide()
{
	//let me be killed by a signal
	unsigned long x = 0;
	*((int *) x) = 1;
}

static void child_func(unsigned long addr)
{
	if (absys_thread_control(AB_SET_ME_SPECIAL)) {
		printf("[PID %lu] set special thread failed.\n", 
		       (unsigned long)getpid());
		suicide();
	}
	//wait parent 
	sleep(5);
	printf("doing some work...\n");
	sleep(5);
	mprotect((void *) addr, 4096, PROT_READ|PROT_WRITE);
	//printf("work done\n");
/*	int a;
	unsigned long *p = (unsigned long *) (addr -4096);
	a = *p; //read 
	printf("[PID %lu] read from addr (%lx): %lx\n",(unsigned long)getpid(), (unsigned long)p, *p);
	*p = a; //write
	printf("[PID %lu] write to addr (%lx): %lx\n",(unsigned long)getpid(), (unsigned long)p, *p);
*/

	sleep(1000);
	//loop
	//for(;;);
}


extern int mapping_test()
{
	int i, status;
	int count = 0;
	int normal = 0;
	pid_t pid[NUM_THREADS], wpid;
	unsigned long addr;
	unsigned long addr_to_map, addr_to_brk;
	void *ret[NUM_THREADS];

	if (absys_thread_control(AB_SET_ME_ARBITER)) {
		printf("set arbiter failed! %d\n",absys_thread_control(AB_SET_ME_ARBITER));
		goto test_failed;
	}
     
	addr = (unsigned long)sbrk(0) + 20*4096;
	printf("sbrk(0) = %lx, addr = %lx.\n", (unsigned long)sbrk(0), addr);
	
	addr_to_map = (addr - 4096) & 0xfffff000; 
	printf("mapping to address %lx.\n", addr_to_map);

	addr_to_brk = 0x80003000;
	
	for (i = 0; i < NUM_THREADS; ++i) {
		pid[i] = fork();
		if (pid[i] < 0) {
			perror("thread cannot be created.\n");
			exit(0);
		}
		if(pid[i] == 0) {
			printf("child process %i created, pid = %lu.\n", i, (unsigned long)getpid());
			child_func(addr_to_brk);
			exit(0);
		}		
	}
	//wait some time for all the child to start
	sleep(2);

	
	//brk((void *)addr);
	//*((unsigned long *)addr_to_map) = 0xdeadbeef;
	//printf("touch the memory and set value %lx.\n", *((unsigned long *)addr_to_map));

	// pick one child
	//ret[0] = (void *)absys_mmap(pid[1], (void *) addr_to_map, 4096, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_FIXED|MAP_PRIVATE, -1, 0);
	//ret[0] = (void *)absys_mmap(pid[1], (void *) (addr_to_map-4096), 4096, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_FIXED|MAP_PRIVATE, -1, 0);
	//ret[1] = (void *)absys_mmap(pid[2], (void *) addr_to_map, 4096, PROT_READ, MAP_ANONYMOUS|MAP_FIXED|MAP_SHARED, -1, 0);
	//ret[2] = (void *)absys_mmap(pid[3], (void *) (addr_to_map-4096), 2*4096, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_FIXED|MAP_SHARED, -1, 0);
	//ret[3] = (void *)absys_mmap(pid[4], (void *) (addr_to_map-4096), 2*4096, PROT_READ, MAP_ANONYMOUS|MAP_FIXED|MAP_SHARED, -1, 0);
	// use mmap as a comparison
	//ret[0] = (void *)mmap((void *) addr_to_map, 4096, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_FIXED|MAP_SHARED, -1, 0);
	//ret[1] = (void *)mmap((void *) (addr_to_map-4096), 4096, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_FIXED|MAP_SHARED, -1, 0);
	//printf("absys_mmap returns %lx.\n", ret[0]==ret[1]?ret[0]:0);

	addr = absys_brk(pid[0], addr_to_brk);
	addr = absys_brk(pid[1], addr_to_brk);
	printf("ret[0] = %lx\n", addr);
	//printf("ret[0] = %lx, ret[1] = %lx\n", ret[0], ret[1]);
	//printf("value %lx.\n", *((unsigned long *)addr_to_map));

	while(1) {
		wpid = waitpid(-1, &status, 0);
		if(wpid <= 0)
			break;
		count++;
		if(WIFEXITED(status)) {
			printf("child %lu exited normally. count = %d\n", 
			       wpid, count);
			normal++;
		}
		if(WIFSIGNALED(status)) {
			printf("child %lu terminated by a signal. count = %d\n",
			       wpid, count);
		}
		else
			printf("wait pid returns with status %d\n", status);
	}
	
	
	if (normal == NUM_THREADS) {
		printf("mapping test successful.\n");
		return 0;
	}


test_failed:
	printf("test failed!!\n");
	return -1;			  
}
