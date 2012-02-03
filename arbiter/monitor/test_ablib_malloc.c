#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>

#include <ab_api.h>
#include <ab_os_interface.h>
#include "ablib_malloc.h" /* ablib_malloc() */
#include "label.h" /* label */

//number of child thread
#define NUM_THREADS 1

static void suicide()
{
	//let me be killed by a signal
	unsigned long x = 0;
	*((int *) x) = 1;
}

static void child_func(unsigned long addr, int i)
{
	void *ret;
	int buf;
	if (absys_thread_control(AB_SET_ME_SPECIAL)) {
		printf("[PID %lu] set special thread failed.\n", 
		       (unsigned long)getpid());
		suicide();
	}
	
	//wait parent 
	sleep(5);
/*	
	//thread 0 initialize data
	if (i == 0) {
		//note
		printf("===============================\n");
		printf("thread 0 has the sensitive data.\nthread 1, 2, 3 could be bad guy.\nthread 4 is a good guy.\n");
		printf("===============================\n");
		printf("thread %d: initialize data...\n", i);
		*(unsigned long *)addr = 0xdeadbeef;
		buf = *(unsigned long *)addr;
		printf("thread %d: init done. the data is %x\n", i, buf);
	}
	//wait thread 0
	sleep(2);
	
	//thread 1 try to read directly. it should fail
	if (i == 1) {
		printf("\nthread %d: try to read directly...\n", i);
		buf = *(unsigned long *)addr;
		printf("thread %d: attack succeed, data is %x.\n", i, buf);
	}
	//wait thread 1
	sleep(2);
	
	//thread 2 try to modify protection using mprotect(). it shoud fail
	if (i == 2) {
		printf("\nthread %d: try to change permission using mprotect()...\n", i);
		if (mprotect((void *) addr, 4096, PROT_READ|PROT_WRITE) == 0) {
			buf = *(unsigned long *)addr;
			printf("thread %d: attack succeed, data is %x.\n", i, buf);
		}
		else {
			printf("thread %d: attack failed.\n", i);
		}
	}
	//wait thread 2
	sleep(2);
	
	//thread 3 try to unmap the region first, and then map it again. but the new mapping is not the same one
	if (i == 3) {
		printf("\nthread %d: step 1 - unmap the region using munmap()...\n", i);

		if (munmap((void *)addr, 4096) == 0) {
			printf("thread %d: step 1 - unmap done.\n", i);
		}
		printf("thread %d: step 2 - map the same region using mmap() with RW privilege...\n", i);
		ret = (void *)mmap((void *)addr, 4096, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_FIXED|MAP_SHARED, -1, 0);
		printf("thread %d: step 2 - map done, now try to read...\n", i);
		buf = *(unsigned long *)addr;
		printf("thread %d: attack succeed, data is %x.\n", i, buf);
	}
	//wait thread 3
	sleep(2);
	
	//thread 4 can read it correctly
	if (i == 4) {
		printf("\nthread %d: try to read directly...\n", i);
		buf = *(unsigned long *)addr;
		printf("thread %d: data is %x.\n", i, buf);
	}
*/
	
/*	int a;
	unsigned long *p = (unsigned long *) (addr -4096);
	a = *p; //read 
	printf("[PID %lu] read from addr (%lx): %lx\n",(unsigned long)getpid(), (unsigned long)p, *p);
	*p = a; //write
	printf("[PID %lu] write to addr (%lx): %lx\n",(unsigned long)getpid(), (unsigned long)p, *p);
*/

	sleep(1000);
}


int main()
{
	int i, status;
	int count = 0;
	int normal = 0;
	pid_t pid[NUM_THREADS], wpid;
	unsigned long addr, len;
	unsigned long addr_to_map, addr_to_brk;
	void *ret[NUM_THREADS];
	cat_t ar = create_category(CAT_S);
        cat_t aw = create_category(CAT_I);       
        label_t L = {ar, aw};

	if (absys_thread_control(AB_SET_ME_ARBITER)) {
		printf("set arbiter failed! %d\n", 
			absys_thread_control(AB_SET_ME_ARBITER));
		goto test_failed;
	}
	
	for (i = 0; i < NUM_THREADS; ++i) {
		pid[i] = fork();
		if (pid[i] < 0) {
			perror("thread cannot be created.\n");
			exit(0);
		}
		if (pid[i] == 0) {
			printf("child process %i created, pid = %lu.\n",
				 i, (unsigned long)getpid());
			child_func(addr_to_map,i);
			exit(0);
		}		
	}
	//wait some time for all the child to start
	sleep(5);

	// test code for ablib_brk()
	printf("chile pid = %d\n", pid[0]);

	addr = (unsigned long)ablib_sbrk(pid[0], 0);
	printf("%lx\n", addr);

	mmap((void *) addr, 10*1024*4, PROT_READ|PROT_WRITE, 
		MAP_ANONYMOUS|MAP_FIXED|MAP_SHARED, -1, 0);	
	touch_mem((void *)addr, 10*1024*4);
	
	addr = (unsigned long)ablib_malloc(pid[0], 1024, L);
	printf("%lx\n", addr);

	addr = (unsigned long)ablib_sbrk(pid[0], 0);
	printf("%lx\n", addr);
	
	addr = (unsigned long)ablib_malloc(pid[0], 10*1024*4, L);
	printf("%lx\n", addr);
	
	addr = (unsigned long)ablib_malloc(pid[0], 10*1024*4, L);
	printf("%lx\n", addr);

	addr = (unsigned long)ablib_sbrk(pid[0], 0);
	printf("%lx\n", addr);

	while(1) {
		wpid = waitpid(-1, &status, 0);
		if(wpid <= 0)
			break;
		count++;
		if(WIFEXITED(status)) {
			printf("child %lu exited normally. count = %d\n", 
			       (unsigned long)wpid, count);
			normal++;
		}
		if(WIFSIGNALED(status)) {
			printf("child %lu terminated by a signal. count = %d\n",
			       (unsigned long)wpid, count);
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
