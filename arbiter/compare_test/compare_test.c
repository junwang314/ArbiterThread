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
#include <lib/timer.h>

/* evaluation parameters */
//iterations for malloc, calloc, realloc, free, get_label,...
#define	NUM 100
//iterations for pthread_create, pthread_join, pthread_self
#define NUM_THREADS 1

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
	
	// test code for pthread_create() and pthread_join()
	struct child_arg *arg_p;
	arg_p = (struct child_arg *)arg;
	unsigned long _addr = arg_p->_addr;
	void *addr_to_map = arg_p->addr_to_map;
	int i = arg_p->i;

	sleep(1);
	return;
}


int main()
{
	int i;
	FILE *fp;

	if ((fp = fopen("result.txt", "w")) == NULL ) {
		printf("Failed to open file!\n");
		exit(1);
	}

	// test code for ulibc_malloc() and ulibc_free()
	void *addr_list[NUM];
	printf("ulibc_malloc %d times...", NUM);
	fprintf(fp, "ulibc_malloc %d times...", NUM);
	start_timer();
	for (i = 0; i < NUM; i++) {
		addr_list[i] = (void *)ulibc_malloc(i*1000);
		AB_DBG("ulibc_malloc: %p, size = %d\n", addr_list[i], i*1000);
	}
	stop_timer(NUM, fp);

	printf("ulibc_free %d times...", NUM);
	fprintf(fp, "ulibc_free %d times...", NUM);
	start_timer();
	for (i = 0; i < NUM; i++) {
		ulibc_free(addr_list[i]);
		AB_DBG("ulibc_free: %p\n", addr_list[i]);
	}
	stop_timer(NUM, fp);

	printf("ulibc_malloc again %d times...", NUM);
	fprintf(fp, "ulibc_malloc again %d times...", NUM);
	start_timer();
	for (i = 0; i < NUM; i++) {
		addr_list[i] = ulibc_malloc(i*1000);
		AB_DBG("ulibc_malloc: %p, size = %d\n", addr_list[i], i*1000);
	}
	stop_timer(NUM, fp);

	// test code for ulibc_realloc()
	printf("ulibc_realloc %d times...", NUM);
	fprintf(fp, "ulibc_realloc %d times...", NUM);
	start_timer();
	for (i = 0; i < NUM; i++) {
		addr_list[i] = ulibc_realloc(addr_list[i], (i%2) ? i*990 : i*1010);
		AB_DBG("ulibc_realloc: %p, size = %d\n", addr_list[i], (i%20) ? i*990 : i*1010);
	}
	stop_timer(NUM, fp);

//	for (i = 0; i < NUM; i++) {
//		ulibc_free(addr_list[i]);
//		AB_DBG("ulibc_free: %p\n", addr_list[i]);
//	}

	// test code for ulibc_calloc()
	printf("ulibc_calloc %d times...", NUM);
	fprintf(fp, "ulibc_calloc %d times...", NUM);
	start_timer();
	for (i = 0; i < NUM; i++) {
		addr_list[i] = ulibc_calloc(i*10, 100);
		AB_DBG("ulibc_calloc: %p, size = %d\n", addr_list[i], i*10*100);
	}
	stop_timer(NUM, fp);


	// test code for pthread_create()
	pthread_t tid[NUM_THREADS];
	struct child_arg tdata[NUM_THREADS];

	printf("pthread_create %d times...", NUM_THREADS);
	fprintf(fp, "pthread_create %d times...", NUM_THREADS);
	fflush(stdout);
	start_timer();
	for (i = 0; i < NUM_THREADS; i++) {
		pthread_create(&tid[i], NULL, child_func, &tdata[i]);
	}
	stop_timer(NUM_THREADS, fp);

	// test code for pthread_self()
	pthread_t mytid;

	printf("pthread_self %d times...", NUM_THREADS);
	fprintf(fp, "pthread_self %d times...", NUM_THREADS);
	start_timer();
	for (i = 0; i < NUM; i++) {
		mytid = pthread_self();
	}
	stop_timer(NUM_THREADS, fp);

	// test code for pthread_join()
	sleep(1);
	int normal = 0;
	printf("pthread_join %d times...", NUM_THREADS);
	fprintf(fp, "pthread_join %d times...", NUM_THREADS);
	start_timer();
	for (i = 0; i < NUM_THREADS; i++) {
		AB_DBG("main: tid[%d] = %d\n", i, (int)tid[i]);
		if (pthread_join(tid[i], NULL) == 0) {
			normal++;
		}
	}
	stop_timer(NUM_THREADS, fp);
	
	fclose(fp);
	if (normal == NUM_THREADS) {
		printf("test successful.\n");
		return 0;
	}


test_failed:
	printf("test failed!!\n");
	return -1;			

}
