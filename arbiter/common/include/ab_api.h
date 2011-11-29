#ifndef _AB_API_H
#define _AB_API_H

#define NUM_CAT_S	4;
#define NUM_CAT_I	4;

#define CAT_S	0x00000000;
#define CAT_I	0x00000001;

typedef unsigned int cat_t;	// category type (e.g. Pr, Pw, ...)
typedef unsigned int cat_type;	// type of category: CAT_S-secrecy, CAT_I-integrity

typedef struct {		// label structure
	cat_t S[NUM_CAT_S];
	cat_t I[NUM_CAT_I];
}label_t;

typedef struct {		// capability set structure
	cat_t cap[NUM_CAT_S + NUM_CAT_I];
}capset;

/* create a category */
cat_t create_category(cat_type t);	// t is either CAT_S or CAT_I 

/* set label for itself */
int self_set_label(label_t L);

/* get label of itself */
label_t get_label(void);		

/* memory allocation */
void *ab_malloc(size_t size, label_t L);

/* get the label of a memory object */
label_t get_mem_label(void *ptr);

/* free memory*/
void ab_free(void *ptr);

/* create new process in the thread control group */
pid_t ab_fork(label_t L, capset O);

/* get the capability set of itself */
capset get_capset();

/* copy and relabel a memeory object */
void *ab_memcp(void *ptr, label_t L);

#endif //_AB_API_H
