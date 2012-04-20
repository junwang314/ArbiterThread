#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h> /* errno */
#include <bits/pthreadtypes.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <ab_api.h>
#include <ab_os_interface.h>
#include <ab_debug.h>
#include <lib_client.h>

//number of child thread
#define NUM_THREADS 4

//DEMO
//#define DEMO
//#define DEMO2

//pthread_mutex_t *mutex;
pthread_cond_t *cond;

// wrappers for Pthread cond operation
void unix_error(char *msg)	// Unix-style error
{
	fprintf(stderr, "%s: %s\n", msg, strerror(errno));
	exit(0);
}

void Pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
{
	if (pthread_mutex_init(mutex, attr) != 0)
		{ fprintf(stderr, "Pthread_mutex_init error\n"); }
}

void Pthread_mutexattr_init(pthread_mutexattr_t *attr)
{
	if (pthread_mutexattr_init(attr) != 0)
		{ fprintf(stderr, "Pthread_mutexattr_init error\n"); }
}

void Pthread_mutexattr_setpshared(pthread_mutexattr_t *attr, int pshared)
{
	if (pthread_mutexattr_setpshared(attr, pshared) != 0)
		{ fprintf(stderr, "Pthread_mutexattr_setpshared error\n"); }
}

void Pthread_mutex_lock(pthread_mutex_t *mutex)
{
	if (pthread_mutex_lock(mutex) != 0)
		{ fprintf(stderr, "Pthread_mutex_lock error\n"); }
}

void Pthread_mutex_trylock(pthread_mutex_t *mutex)
{
	if (pthread_mutex_trylock(mutex) != 0)
		{ fprintf(stderr, "Pthread_mutex_trylock error\n"); }
}

void Pthread_mutex_unlock(pthread_mutex_t *mutex)
{
	if (pthread_mutex_unlock(mutex) != 0)
		{ fprintf(stderr, "Pthread_mutex_unlock error\n"); }
}

void Pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr)
{
	if (pthread_cond_init(cond, attr) != 0)
		{ fprintf(stderr, "Pthread_cond_init error\n"); }
}

void Pthread_condattr_init(pthread_condattr_t *attr)
{
	if (pthread_condattr_init(attr) != 0)
		{ fprintf(stderr, "Pthread_condattr_init error\n"); }
}

void Pthread_condattr_setpshared(pthread_condattr_t *attr, int pshared)
{
	if (pthread_condattr_setpshared(attr, pshared) != 0)
		{ fprintf(stderr, "Pthread_condattr_setpshared error\n"); }
}

void Pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
	if (pthread_cond_wait(cond, mutex) != 0)
		{ fprintf(stderr, "Pthread_cond_wait error\n"); }
}

void Pthread_cond_signal(pthread_cond_t *cond)
{
	if (pthread_cond_signal(cond) != 0)
		{ fprintf(stderr, "Pthread_cond_signal error\n"); }
}

static void suicide()
{
	//let me be killed by a signal
	unsigned long x = 0;
	*((int *) x) = 1;
}

static void debug_mutex(pthread_mutex_t *mutex)
{
	AB_DBG("__lock = %d\n", (&(mutex->__data))->__lock);
	return;
}

struct child_arg {
	unsigned long _addr;
	void *addr_to_map;
	label_t L1;
	label_t L2; 
	int i;
};

void * child_func(void *arg)
{
	void *ret;
	int buf;
	unsigned long addr;
	int j;
	
	// test code for ab_pthread_create() and ab_pthread_join()
	struct child_arg *arg_p;
	arg_p = (struct child_arg *)arg;
	unsigned long _addr = arg_p->_addr;
	void *addr_to_map = arg_p->addr_to_map;
	int i = arg_p->i;

	//wait parent 
	sleep(5);

	if (i == 0) {
#ifndef DEMO
		//addr = (unsigned long)ab_malloc(4, L1);
		//printf("child B malloc: %lx\n", addr);

		//*(unsigned long *)_addr = 0xdeadbeef;
		//printf("child B: *_addr = %lx\n", *(unsigned long *)_addr);		
		//test code for pthread mutex
		pthread_mutex_t *mutex;
		pthread_mutexattr_t attr;

		mutex = (pthread_mutex_t *)addr_to_map;
		AB_DBG("child B: mutex = %p\n", mutex);
		for (j = 0; j < 1; j++) {
			AB_DBG("child B: ");
			debug_mutex(mutex);
			sleep(1);
		}
		Pthread_mutex_lock(mutex);
		printf("child B: doing somthing\n");
		//Pthread_cond_signal(cond);
		Pthread_mutex_unlock(mutex);
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
	//return;
	//sleep(30);
	return;
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
	own_t O_self;
	
	addr = (unsigned long)ab_calloc(256, 4, L1);
	printf("child A malloc: %lx\n", addr);
	//printf("addr = %lx\n", *(unsigned long *)addr);
	//*(unsigned long *)addr = 0x12345678;
	
	// test code for get_ownership()
/*	get_ownership(O_self);
	print_label(O_self);
*/
	// test code for pthread mutex and condition variable
	pthread_mutex_t *mutex;
	pthread_mutexattr_t attr;
	
	mutex = (pthread_mutex_t *)ab_malloc(sizeof(pthread_mutex_t), L1);
	Pthread_mutexattr_init(&attr);
	Pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
	Pthread_mutex_init(mutex, &attr);

	cond = (pthread_cond_t *)ab_malloc(sizeof(pthread_cond_t), L1);
	pthread_condattr_t condattr;
	Pthread_condattr_init(&condattr);
	Pthread_condattr_setpshared(&condattr, PTHREAD_PROCESS_SHARED);
	Pthread_cond_init(cond, &condattr);

	// test code for ab_pthread_create() and ab_pthread_join()
	pthread_t tid[NUM_THREADS];
	struct child_arg tdata[NUM_THREADS];
	
	for (i = 0; i < NUM_THREADS; ++i) {
		if (i == 0) {
			tdata[i]._addr = addr;
			tdata[i].addr_to_map = (void *)mutex;
			tdata[i].i = i;
			ab_pthread_create(&tid[i], NULL, child_func, &tdata[i], L1, O);
		}
		if (i == 1) {
			tdata[i].i = i;
			ab_pthread_create(&tid[i], NULL, child_func, &tdata[i], L2, O);
		}
		if (i == 2) {
			tdata[i].i = i;
			ab_pthread_create(&tid[i], NULL, child_func, &tdata[i], (label_t){}, (own_t){});
		}
		if (i == 3) {
			tdata[i].i = i;
			ab_pthread_create(&tid[i], NULL, child_func, &tdata[i], (label_t){}, O);
		}
	}
	//wait some time for all the child to start
	//sleep(10);
	//printf("child A: addr = %lx\n", *(unsigned long *)addr);
	//*(unsigned long *)addr = 0xdeadbeef;
	// test code for pthread mutex
	AB_DBG("child A: mutex = %p\n", mutex);
	AB_DBG("child A: ");
	debug_mutex(mutex);
	Pthread_mutex_lock(mutex);
	//Pthread_cond_wait(cond, mutex);
	AB_DBG("child A: ");
	debug_mutex(mutex);
	printf("child A: doing somthing\n");
	sleep(10);	
	Pthread_mutex_unlock(mutex);	
	AB_DBG("child A: ");
	debug_mutex(mutex);
	// test code for ablib_brk()
//	addr = (unsigned long)ablib_sbrk(pid[0], 0);
//	printf("child 0 sbrk: %lx\n", addr);
	
	// test code for ab_malloc()
/*	addr = (unsigned long)ab_malloc(41000, L2);
	printf("child A malloc: %lx\n", addr);
	//ab_free((void *)addr);
	addr = (unsigned long)ab_malloc(11*1024*4, L2);
	printf("child A malloc: %lx\n", addr);
*/
	// test code for ab_malloc() and ab_free() exhaustively
/*	unsigned long addr_list[100];
	for (i = 0; i < 100; i = i + 1) {
		addr_list[i] = (unsigned long)ab_malloc(i*1000, L2);
		printf("child A malloc: %lx, size = %d\n", addr_list[i], i*1000);
	}
	for (i = 0; i < 100; i = i + 1) {
		ab_free((void *)addr_list[i]);
		printf("child A free: %lx\n", addr_list[i]);
	}
	for (i = 0; i < 100; i = i + 1) {
		addr_list[i] = (unsigned long)ab_malloc(i*1000, L2);
		printf("child A malloc: %lx, size = %d\n", addr_list[i], i*1000);
	}
*/
	// test code for get_mem_label()
/*	addr = (unsigned long)ab_malloc(4, L2);
	printf("child A malloc: %lx\n", addr);
	print_label(L2);
	get_mem_label((void *)addr, L_test);
	print_label(L_test);
*/

	// test code for get_label() & get_mem_label();
/*	get_label(L_self);
	print_label(L_self);
	addr = (unsigned long)ab_malloc(4, L_self);
	printf("child A malloc: %lx\n", addr);

	get_mem_label((void *)addr, L_test);
	print_label(L_test);
*/	
	
/*	while(1) {
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
*/	
	// test code for ab_pthread_create() and ab_pthread_join()
	for (i = 0; i < NUM_THREADS; ++i) {
		AB_DBG("main: tid[%d] = %d\n", i, tid[i]);
		if (ab_pthread_join(tid[i], NULL) == 0) {
			normal++;
		}
	}
	
	if (normal == NUM_THREADS) {
		printf("mapping test successful.\n");
		return 0;
	}


test_failed:
	printf("test failed!!\n");
	return -1;			

}
