#ifndef _AB_API_H
#define _AB_API_H

#include <stdint.h> /* uint8_t */
#include <unistd.h> /* pid_t */
#include <pthread.h>

#define CAT_S	((uint8_t)0b00000000)
#define CAT_I	((uint8_t)0b10000000)	// use most significant bit as flag

typedef uint8_t cat_t;		// category type (e.g. Pr, Pw, ...)
typedef uint8_t cat_type;	// type of category: CAT_S-secrecy, CAT_I-integrity
typedef cat_t label_t[8];	// label (e.g. {Pr, Qw})
typedef cat_t own_t[8];		// capability set structure

/* create a category */
cat_t create_category(cat_type t);	// t is either CAT_S or CAT_I 

/* create new process in the thread control group */
pid_t ab_fork(label_t L, own_t O);

/* create new thread in the thread control group */
int ab_pthread_create(pthread_t *thread, const pthread_attr_t *attr, 
		      void * (*start_routine)(void *), void *arg, 
		      label_t L, own_t O);

//old version, fork() implementation
int ab_pthread_create_old(pthread_t *thread, const pthread_attr_t *attr, 
			  void * (*start_routine)(void *), void *arg, 
			  label_t L, own_t O);

/* wait for thread termination */
int ab_pthread_join(pthread_t thread, void **value_ptr);

/* get label of itself into L */
void get_label(label_t L);		

/* get the capability set of itself into O */
void get_ownership(own_t O);

/* memory allocation */
void *ab_malloc(size_t size, label_t L);

/* free memory*/
void ab_free(void *ptr);

/* get the label of a memory object */
void get_mem_label(void *ptr, label_t L);

/* memory allocation */
void *ab_calloc(size_t nmemb, size_t size, label_t L);

static void print_label(label_t L)
{
	int i;
	for (i = 0; i < 8; i++) {
		printf("L[%d]=%hx  ", i, L[i]);
	}
	printf("\n");
}

#endif //_AB_API_H
