#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>

#include <ab_api.h>
#include <ab_os_interface.h>
#include <ab_debug.h>
#include <lib_client.h>

//number of child thread
#define NUM_THREADS 4

//DEMO
//#define DEMO
//#define DEMO2

static void suicide()
{
	//let me be killed by a signal
	unsigned long x = 0;
	*((int *) x) = 1;
}

static void child_func(unsigned long addr_to_map, label_t L1, label_t L2, int i)
{
	void *ret;
	int buf;
	unsigned long addr;
	
	//wait parent 
	sleep(10);

	if (i == 0) {
#ifndef DEMO
		//addr = (unsigned long)ab_malloc(4, L1);
		//printf("child B malloc: %lx\n", addr);
#endif
#ifdef DEMO
		//child B initialize data
		//note
		printf("===============================\n");
		printf("child B has the sensitive data.\nchild D could be bad guy.\nchild C is a good guy.\n");
		printf("===============================\n");
		printf("child B: initialize data...\n");
		addr = addr_to_map;
		*(unsigned long *)addr = 0xdeadbeef;
		buf = *(unsigned long *)addr;
		printf("child B: init done. the data is %x\n", buf);
#endif
	}
#ifdef DEMO1
	//wait thread 0
	sleep(2);
	
	//child D try to read directly. it should fail
	if (i == 2) {
		printf("\nchild D: try to read directly...\n");
		addr = addr_to_map;
		buf = *(unsigned long *)addr;
		printf("child D: attack succeed, data is %x.\n", buf);
	}
#endif
#ifdef DEMO2
	//wait thread 1
	sleep(2);
	
	//child D try to modify protection using mprotect(). it shoud fail
	if (i == 2) {
		printf("\nchild D: try to change permission using mprotect()...\n");
		addr = addr_to_map;
		if (mprotect((void *) addr, 4096, PROT_READ|PROT_WRITE) == 0) {
			buf = *(unsigned long *)addr;
			printf("child D: attack succeed, data is %x.\n", buf);
		}
		else {
			printf("child D: attack failed.\n");
		}
	}
#endif
#ifdef DEMO3
	//wait thread 2
	sleep(2);
	
	//child D try to unmap the region first, and then map it again. but the new mapping is not the same one
	if (i == 2) {
		printf("\nchild D: step 1 - unmap the region using munmap()...\n");
		addr = addr_to_map;
		if (munmap((void *)addr, 4096) == 0) {
			printf("child D: step 1 - unmap done.\n");
		}
		printf("child D: step 2 - map the same region using mmap() with RW privilege...\n");
		ret = (void *)mmap((void *)addr, 4096, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_FIXED|MAP_SHARED, -1, 0);
		printf("child D: step 2 - map done, now try to read...\n");
		buf = *(unsigned long *)addr;
		printf("child D: attack succeed, data is %x.\n", buf);
	}
#endif
#ifdef DEMO
	//wait thread 3
	sleep(2);
	
	//child C can read it correctly
	if (i == 1) {
		printf("\nchild C: try to read directly...\n", i);
		addr = addr_to_map;
		buf = *(unsigned long *)addr;
		printf("child C: data is %x.\n", buf);
	}
#endif
/*	int a;
	unsigned long *p = (unsigned long *) (addr -4096);
	a = *p; //read 
	printf("[PID %lu] read from addr (%lx): %lx\n",(unsigned long)getpid(), (unsigned long)p, *p);
	*p = a; //write
	printf("[PID %lu] write to addr (%lx): %lx\n",(unsigned long)getpid(), (unsigned long)p, *p);
*/

	sleep(1000);
}


int client_test()
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
        label_t L1 = {ar, aw};
        label_t L2 = {ar};
	own_t O = {};
	label_t L_self, L_test;
	
	addr = (unsigned long)ab_malloc(1*1024*4, L1);
	printf("child A malloc: %lx\n", addr);

	// test code for get_ownership()
	own_t O_self;
	get_ownership(O_self);
	print_label(O_self);

	addr_to_map = 0x80000000;
	for (i = 0; i < NUM_THREADS; ++i) {
		if (i == 0)
			pid[i] = ab_fork(L1, O);
		if (i == 1)
			pid[i] = ab_fork(L2, O);
		if (i == 2)
			pid[i] = ab_fork((label_t){}, (own_t){});
		if (i == 3)
			pid[i] = ab_fork((label_t){}, O_self);
		if (pid[i] < 0) {
			perror("thread cannot be created.\n");
			exit(0);
		}
		if (pid[i] == 0) {
			printf("child process %i created, pid = %lu.\n",
				 i, (unsigned long)getpid());
			child_func(addr_to_map, L1, L2, i);
			exit(0);
		}		
	}
	//wait some time for all the child to start
	//sleep(5);

	// test code for ablib_brk()
//	addr = (unsigned long)ablib_sbrk(pid[0], 0);
//	printf("child 0 sbrk: %lx\n", addr);
	
//	addr = (unsigned long)ab_malloc(1*1024*4, L2);
//	printf("child A malloc: %lx\n", addr);
	//ab_free((void *)addr);
/*	for (i = 1; i < 100*1024*4; i = i + 100) {
		addr = (unsigned long)ab_malloc(i%(50*1024*4), L2);
		printf("child A malloc: %lx, size = %d\n", addr, i%(50*1024*4));
	
		ab_free((void *)addr);
		//printf("child A free: %lx\n", addr);
	}
*/	
	addr = (unsigned long)ab_malloc(4, L2);
	printf("child A malloc: %lx\n", addr);
	print_label(L2);
	get_mem_label((void *)addr, L_test);
	print_label(L_test);

	//addr = (unsigned long)ablib_malloc(pid[0], 1024*4, L1);
	//printf("child 0 malloc: %lx\n", addr);
	
	//addr = (unsigned long)ablib_malloc(pid[0], 9*1024*4, L1);
	//printf("child 0 malloc: %lx\n", addr);

	//addr = (unsigned long)ablib_sbrk(pid[0], 0);
	//printf("child 0 sbrk: %lx\n", addr);
	
	//addr = (unsigned long)ablib_sbrk(pid[1], 0);
	//printf("child 1 sbrk: %lx\n", addr);

	// test code for get_label() & get_mem_label();
	get_label(L_self);
	print_label(L_self);
	addr = (unsigned long)ab_malloc(4, L_self);
	printf("child A malloc: %lx\n", addr);

	get_mem_label((void *)addr, L_test);
	print_label(L_test);
	
	
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
