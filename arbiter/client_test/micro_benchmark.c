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
#include <lib/timer.h>

/* evaluation parameters */
//iterations for malloc, calloc, realloc, free, get_label,...
#define	NUM 100
//iterations for pthread_create, pthread_join, pthread_self()
#define NUM_THREADS 4

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

	sleep(1);
	return;
}


int client_test()
{
	int i;
	FILE *fp;

	if ((fp = fopen("result.txt", "w")) == NULL ) {
		printf("Failed to open file!\n");
		exit(1);
	}

	// test code for create_category
	int num = 4; //mximum number of category for a thread is 8
	cat_t r[num], w[num];
	printf("create_category(CAT_S) %d times...", num);
	fprintf(fp, "create_category(CAT_S) %d times...", num);
	start_timer();
	for (i = 0; i < num; i++) {
		r[i] = create_category(CAT_S);
	}
	stop_timer(num, fp);

	printf("create_category(CAT_I) %d times...", num);
	fprintf(fp, "create_category(CAT_I) %d times...", num);
	start_timer();
	for (i = 0; i < num; i++) {
		w[i] = create_category(CAT_I);
	}
	stop_timer(num, fp);

	// test code for ab_malloc() and ab_free()
        label_t L1 = {r[0], w[0]};
	void *addr_list[NUM];
	printf("ab_malloc %d times...", NUM);
	fprintf(fp, "ab_malloc %d times...", NUM);
	start_timer();
	for (i = 0; i < NUM; i++) {
		addr_list[i] = (void *)ab_malloc(i*1000, L1);
		AB_DBG("ab_malloc: %p, size = %d\n", addr_list[i], i*1000);
	}
	stop_timer(num, fp);

	printf("ab_free %d times...", NUM);
	fprintf(fp, "ab_free %d times...", NUM);
	start_timer();
	for (i = 0; i < NUM; i++) {
		ab_free(addr_list[i]);
		AB_DBG("ab_free: %p\n", addr_list[i]);
	}
	stop_timer(num, fp);

	printf("ab_malloc again %d times...", NUM);
	fprintf(fp, "ab_malloc again %d times...", NUM);
	start_timer();
	for (i = 0; i < NUM; i++) {
		addr_list[i] = ab_malloc(i*1000, L1);
		AB_DBG("ab_malloc: %p, size = %d\n", addr_list[i], i*1000);
	}
	stop_timer(num, fp);

	// test code for ab_realloc()
	printf("ab_realloc %d times...", NUM);
	fprintf(fp, "ab_realloc %d times...", NUM);
	start_timer();
	for (i = 0; i < NUM; i++) {
		addr_list[i] = ab_realloc(addr_list[i], (i%2) ? i*990 : i*1010);
		AB_DBG("ab_realloc: %p, size = %d\n", addr_list[i], (i%20) ? i*990 : i*1010);
	}
	stop_timer(num, fp);
	for (i = 0; i < NUM; i++) {
		ab_free(addr_list[i]);
		AB_DBG("ab_free: %p\n", addr_list[i]);
	}

	// test code for ab_calloc()
	printf("ab_calloc %d times...", NUM);
	fprintf(fp, "ab_calloc %d times...", NUM);
	start_timer();
	for (i = 0; i < NUM; i++) {
		addr_list[i] = ab_calloc(i*10, 100, L1);
		AB_DBG("ab_calloc: %p, size = %d\n", addr_list[i], i*10*100);
	}
	stop_timer(num, fp);

	// test code for get_mem_label()
	label_t L_list[NUM];
	printf("get_mem_label %d times...", NUM);
	fprintf(fp, "get_mem_label %d times...", NUM);
	start_timer();
	for (i = 0; i < NUM; i++) {
		get_mem_label(addr_list[i], L_list[i]);
	}
	stop_timer(num, fp);

	// test code for get_label()
	printf("get_label %d times...", NUM);
	fprintf(fp, "get_label %d times...", NUM);
	start_timer();
	for (i = 0; i < NUM; i++) {
		get_label(L_list[i]);
	}
	stop_timer(num, fp);
	
	// test code for get_ownership()
	own_t O_list[NUM];
	printf("get_ownership %d times...", NUM);
	fprintf(fp, "get_ownership %d times...", NUM);
	start_timer();
	for (i = 0; i < NUM; i++) {
		get_ownership(O_list[i]);
	}
	stop_timer(num, fp);


	// test code for ab_pthread_create() and ab_pthread_join()
	own_t O1 = {r[0], w[0]};
	pthread_t tid[NUM_THREADS];
	struct child_arg tdata[NUM_THREADS];

	printf("ab_pthread_create %d times...", NUM_THREADS);
	fprintf(fp, "ab_pthread_create %d times...", NUM_THREADS);
	fflush(stdout);
	start_timer();
	for (i = 0; i < NUM_THREADS; i++) {
		ab_pthread_create(&tid[i], NULL, child_func, &tdata[i], L1, O1);
	}
	stop_timer(num, fp);

	// test code for ab_pthread_create() and ab_pthread_join()
	sleep(1);
	int normal = 0;
	printf("ab_pthread_join %d times...", NUM_THREADS);
	fprintf(fp, "ab_pthread_join %d times...", NUM_THREADS);
	start_timer();
	for (i = 0; i < NUM_THREADS; i++) {
		AB_DBG("main: tid[%d] = %d\n", i, (int)tid[i]);
		if (ab_pthread_join(tid[i], NULL) == 0) {
			normal++;
		}
	}
	stop_timer(num, fp);
	
	fclose(fp);
	if (normal == NUM_THREADS) {
		printf("test successful.\n");
		return 0;
	}


test_failed:
	printf("test failed!!\n");
	return -1;			

}
