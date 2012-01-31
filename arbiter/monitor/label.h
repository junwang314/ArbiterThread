#ifndef _LABEL_H
#define _LABEL_H

#include <ab_api.h>

//label check result of memory protection
enum mem_prot {
	PROT_N,
	PROT_R,
	PROT_RW
};

/* check label according to RULE #2-3:
 * - if creating an object capset O2 can be NULL  
 * - return 0 if label check OK (pass), -1 if fail
 */
int check_label(label_t L1, own_t O1, label_t L2, own_t O2);

/* check label according to RULE #1:
 * return one of the three mem_prot result
 */
enum mem_prot check_mem_prot(label_t L1, own_t O1, label_t L2);

#endif //_LABEL_H
