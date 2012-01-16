#ifndef _LABEL_H
#define _LABEL_H

#include <ab_api.h>

/* check label according to RULE #1-3:
 * - if creating an object capset O2 can be NULL  
 * - return 0 if label check OK (pass), -1 if fail
 */
int check_label(label_t L1, capset O1, label_t L2, capset O2);

#endif //_LABEL_H
