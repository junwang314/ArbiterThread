#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <pthread.h>
#include <errno.h> /* errno */


int shmid;

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

static void child_func(void)
{	
	int shmid;
	key_t key;
	char *shm;

	//wait parent 
	sleep(5);

	//init shared memory for mutex
	key = 2222;
	if ((shmid = shmget(key, sizeof(pthread_mutex_t), 0666)) < 0) {
	        perror("shmget");
	        exit(1);
	}
	if ((shm = shmat(shmid, NULL, 0)) == (char *) -1) {
		perror("shmat");
	        exit(1);
	}
	pthread_mutex_t *mutex;
	mutex = (pthread_mutex_t *)shm;
				
	//init shared memory for cond
	key = 5555;
	if ((shmid = shmget(key, sizeof(pthread_cond_t), 0666)) < 0) {
	        perror("shmget");
	        exit(1);
	}
	if ((shm = shmat(shmid, NULL, 0)) == (char *) -1) {
		perror("shmat");
	        exit(1);
	}
	pthread_cond_t *cond;
	cond = (pthread_cond_t *)shm;
	
	//test code for pthread cond
	Pthread_mutex_lock(mutex);
	printf("child: doing somthing\n");
	Pthread_cond_signal(cond);
	Pthread_mutex_unlock(mutex);

	
	return;
}

void main()
{
	int shmid;
	key_t key;
	char *shm;

	//init shared memory for mutex
	key = 2222;
	if ((shmid = shmget(key, sizeof(pthread_mutex_t), IPC_CREAT | 0666)) < 0) {
	        perror("shmget");
	        exit(1);
	}
	if ((shm = shmat(shmid, NULL, 0)) == (char *) -1) {
		perror("shmat");
	        exit(1);
	}
	pthread_mutex_t *mutex;
	pthread_mutexattr_t mutexattr;
	mutex = (pthread_mutex_t *)shm;
	Pthread_mutexattr_init(&mutexattr);
	Pthread_mutexattr_setpshared(&mutexattr, PTHREAD_PROCESS_SHARED);
	Pthread_mutex_init(mutex, &mutexattr);		

	//init shared memory for cond
	key = 5555;
	if ((shmid = shmget(key, sizeof(pthread_cond_t), IPC_CREAT | 0666)) < 0) {
	        perror("shmget");
	        exit(1);
	}
	if ((shm = shmat(shmid, NULL, 0)) == (char *) -1) {
		perror("shmat");
	        exit(1);
	}
	pthread_cond_t *cond;
	pthread_condattr_t condattr;
	cond = (pthread_cond_t *)shm;
	Pthread_condattr_init(&condattr);
	Pthread_condattr_setpshared(&condattr, PTHREAD_PROCESS_SHARED);
	Pthread_cond_init(cond, &condattr);
	
	//fork a child
	pid_t pid;
	pid = fork();
	if (pid < 0) {
		perror("chile cannot be created.\n");
		exit(0);
	}
	if (pid == 0) {
		printf("child process created, pid = %lu.\n", (unsigned long)getpid());
		child_func();
		exit(0);
	}		
	
	Pthread_mutex_lock(mutex);
	Pthread_cond_wait(cond, mutex);
	sleep(15);
	printf("parent: doing somthing\n");	
	Pthread_mutex_unlock(mutex);
	
	return;
}
