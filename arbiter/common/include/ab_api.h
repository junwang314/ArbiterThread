#ifndef _AB_API_H
#define _AB_API_H

#include <stdint.h>

#define CAT_S	0b00000000
#define CAT_I	0b10000000	// use most significant bit as flag

typedef uint8_t cat_t;		// category type (e.g. Pr, Pw, ...)
typedef uint8_t cat_type;	// type of category: CAT_S-secrecy, CAT_I-integrity
typedef cat_t label_t[8];	// label (e.g. {Pr, Qw})
typedef cat_t own_t[8];		// capability set structure

/* create a category */
cat_t create_category(cat_type t);	// t is either CAT_S or CAT_I 

/* create new process in the thread control group */
pid_t ab_fork(label_t L, own_t O);

/* get label of itself */
cat_t *get_label(void);		

/* get the capability set of itself */
cat_t *get_ownership(void);

/* memory allocation */
void *ab_malloc(size_t size, label_t L);

/* free memory*/
void ab_free(void *ptr);

/* get the label of a memory object */
cat_t *get_mem_label(void *ptr);

/* copy and relabel a memeory object */
void *ab_memcpy(void *dest, const void *src, size_t n, label_t L);

#endif //_AB_API_H
