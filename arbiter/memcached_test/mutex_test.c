#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <pthread.h>
#include <errno.h> /* errno */

// wrappers for Pthread mutex operation
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

static void child_func(void)
{	
	int shmid;
	key_t key;
	char *shm;

	//wait parent 
	sleep(10);
		
	//init shared memory
	key = 5678;
	if ((shmid = shmget(key, sizeof(pthread_mutex_t), 0666)) < 0) {
	        perror("shmget");
	        exit(1);
	}
	if ((shm = shmat(shmid, NULL, 0)) == (char *) -1) {
		perror("shmat");
	        exit(1);
	}
	
	//test code for pthread mutex
	pthread_mutex_t *mutex;
	pthread_mutexattr_t attr;
	
	mutex = (pthread_mutex_t *)shm;
	
	printf("child: mutex = %p\n", mutex);
	Pthread_mutex_lock(mutex);
	printf("child: doing somthing\n");
	Pthread_mutex_unlock(mutex);
	
	return;
}

void main()
{
	int shmid;
	key_t key;
	char *shm;

	//init shared memory
	key = 5678;
	if ((shmid = shmget(key, sizeof(pthread_mutex_t), IPC_CREAT | 0666)) < 0) {
	        perror("shmget");
	        exit(1);
	}
	if ((shm = shmat(shmid, NULL, 0)) == (char *) -1) {
		perror("shmat");
	        exit(1);
	}
	
	//init mutex
	pthread_mutex_t *mutex;
	pthread_mutexattr_t attr;
	
	mutex = (pthread_mutex_t *)shm;
	Pthread_mutexattr_init(&attr);
	Pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
	Pthread_mutex_init(mutex, NULL);
	
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
	
	printf("parent: mutex = %p\n", mutex);
	Pthread_mutex_lock(mutex);
	sleep(15);
	printf("parent: doing somthing\n");	
	Pthread_mutex_unlock(mutex);
	
	return;
}
