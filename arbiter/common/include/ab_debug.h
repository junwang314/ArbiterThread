#ifndef _AB_DEBUG_H
#define _AB_DEBUG_H

#include <stdio.h>

/********** debug **********/

#define AB_VERBOSE_TAG 1
#define AB_INFO_TAG  1 
#define AB_DEBUG_TAG  1

#define AB_MSG(...)  if((AB_VERBOSE_TAG)) {printf(__VA_ARGS__);}
#define AB_INFO(...) if((AB_INFO_TAG)) {printf(__VA_ARGS__);}
#define AB_DBG(...)  if((AB_DEBUG_TAG)) {printf(__VA_ARGS__);}


#endif //_AB_DEBUG_H
