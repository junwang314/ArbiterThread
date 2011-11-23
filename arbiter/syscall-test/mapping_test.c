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
	unsigned long addr, len;
	unsigned long addr_to_map, addr_to_brk;
	void *ret[NUM_THREADS];

	if (absys_thread_control(AB_SET_ME_ARBITER)) {
		printf("set arbiter failed! %d\n",absys_thread_control(AB_SET_ME_ARBITER));
		goto test_failed;
	}

	/* test parameter for absys_mmap */     
	//addr = (unsigned long)sbrk(0) + 20*4096;
	//printf("sbrk(0) = %lx, addr = %lx.\n", (unsigned long)sbrk(0), addr);
	//addr_to_map = (addr - 4096) & 0xfffff000; 
	//printf("mapping to address %lx.\n", addr_to_map);

	/* test parameter for absys_brk, absys_mprotect, absys_munmap */
	addr_to_brk = 0x80006000;
	addr_to_map = 0x80003000;
	len = addr_to_brk - addr_to_map;
	
	for (i = 0; i < NUM_THREADS; ++i) {
		pid[i] = fork();
		if (pid[i] < 0) {
			perror("thread cannot be created.\n");
			exit(0);
		}
		if(pid[i] == 0) {
			printf("child process %i created, pid = %lu.\n", i, (unsigned long)getpid());
			child_func(addr_to_map,i);
			exit(0);
		}		
	}
	//wait some time for all the child to start
	sleep(5);

	/* test code for absys_mmap() */
	//brk((void *)addr);
	//*((unsigned long *)addr_to_map) = 0xdeadbeef;
	//printf("touch the memory and set value %lx.\n", *((unsigned long *)addr_to_map));

	// pick one child
	//ret[0] = (void *)absys_mmap(pid[1], (void *) addr_to_map, 4096, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_FIXED|MAP_PRIVATE, -1, 0);
	//ret[0] = (void *)absys_mmap(pid[1], (void *) (addr_to_map-4096), 4096, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_FIXED|MAP_PRIVATE, -1, 0);
	//ret[1] = (void *)absys_mmap(pid[2], (void *) addr_to_map, 4096, PROT_READ, MAP_ANONYMOUS|MAP_FIXED|MAP_SHARED, -1, 0);
	//ret[2] = (void *)absys_mmap(pid[3], (void *) (addr_to_map-4096), 2*4096, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_FIXED|MAP_SHARED, -1, 0);
	//ret[3] = (void *)absys_mmap(pid[4], (void *) (addr_to_map-4096), 2*4096, PROT_READ, MAP_ANONYMOUS|MAP_FIXED|MAP_SHARED, -1, 0);
	/* use mmap as a comparison */
	//ret[0] = (void *)mmap((void *) addr_to_map, 4096, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_FIXED|MAP_SHARED, -1, 0);
	//ret[1] = (void *)mmap((void *) (addr_to_map-4096), 4096, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_FIXED|MAP_SHARED, -1, 0);
	//printf("absys_mmap returns %lx.\n", ret[0]==ret[1]?ret[0]:0);

	/* test parameter for absys_brk, absys_mprotect, absys_munmap */
	mmap((void *) addr_to_map, len, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_FIXED|MAP_SHARED, -1, 0);	
	*((unsigned long *)addr_to_map) = 0;
	
	/* test code for absys_brk */
	addr = absys_brk(pid[0], (void *)addr_to_brk);	
	printf("new brk = %lx\n", addr);
	
	/* test code for absys_mprotect */
	printf("ret = %d\n", absys_mprotect(pid[1], (void *)addr_to_map, 4096, PROT_NONE));
	printf("ret = %d\n", absys_mprotect(pid[2], (void *)addr_to_map, 4096, PROT_NONE));
	printf("ret = %d\n", absys_mprotect(pid[3], (void *)addr_to_map, 4096, PROT_NONE));
	
	/* test code for absys_munmap */
	printf("ret = %d\n", absys_munmap(pid[4], (void *)addr_to_map, 4096));
	
	/* code out of use */
	//ret[0] = (void *)absys_mmap(pid[0], (void *) (0xffff1000), 2*4096, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_FIXED|MAP_PRIVATE, -1, 0);
	//ret[0] = (void *)mmap((void *) (addr_to_brk+2*4096), 2*4096, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_FIXED|MAP_PRIVATE, -1, 0);
	//printf("ret = %lx\n", (unsigned long)ret[0]);
	
	//addr = absys_brk(pid[1], ret[0] + 4096);
	//printf("ret = %lx\n", addr);
	
	//printf("ret[0] = %lx, ret[1] = %lx\n", ret[0], ret[1]);
	//printf("value %lx.\n", *((unsigned long *)addr_to_map));

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
