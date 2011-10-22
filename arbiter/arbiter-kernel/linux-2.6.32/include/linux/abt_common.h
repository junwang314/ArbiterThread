#ifndef _ABT_COMMON_H
#define _ABT_COMMON_H

#include <linux/kernel.h>


/********** debug **********/

#define AB_VEBOSE_TAG 1
#define AB_INFO_TAG   1
#define AB_DEBUG_TAG  1

#define AB_MSG(...)  if((AB_VERBOSE_TAG)) {printk(KERN_ERR __VA_ARGS__);}
#define AB_INFO(...) if((AB_INFO_TAG)) {printk(KERN_ERR __VA_ARGS__);}
#define AB_DBG(...)  if((AB_DEBUG_TAG)) {printk(KERN_ERR __VA_ARGS__);}






#endif //_ABT_COMMON_H
