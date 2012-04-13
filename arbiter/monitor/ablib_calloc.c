/*
  This is a version (aka dlmalloc) of malloc/free/realloc written by
  Doug Lea and released to the public domain.  Use, modify, and
  redistribute this code without permission or acknowledgement in any
  way you wish.  Send questions, comments, complaints, performance
  data, etc to dl@cs.oswego.edu

  VERSION 2.7.2 Sat Aug 17 09:07:30 2002  Doug Lea  (dl at gee)

  Note: There may be an updated version of this malloc obtainable at
           ftp://gee.cs.oswego.edu/pub/misc/malloc.c
  Check before installing!

  Hacked up for uClibc by Erik Andersen <andersen@codepoet.org>
*/

#include "ablib_malloc.h"


/* ------------------------------ calloc ------------------------------ */
void* ablib_calloc(pid_t pid, size_t n_elements, size_t elem_size, label_t L)
{
    mchunkptr p;
    unsigned long  clearsize;
    unsigned long  nclears;
    size_t size, *d;
    void* mem;


    AB_INFO("ablib_calloc called: arguments = (%d, %d, %d, %lx)\n", pid, n_elements, elem_size, *(unsigned long *)L);
    /* guard vs integer overflow, but allow nmemb
     * to fall through and call malloc(0) */
    size = n_elements * elem_size;
    if (n_elements && elem_size != (size / n_elements)) {
	__set_errno(ENOMEM);
	return NULL;
    }

//    __MALLOC_LOCK;	//TODO: ask Xi
    mem = ablib_malloc(pid, size, L);
    if (mem != 0) {
	p = mem2chunk(mem);

	if (!chunk_is_mmapped(p))
	{
	    /*
	       Unroll clear of <= 36 bytes (72 if 8byte sizes)
	       We know that contents have an odd number of
	       size_t-sized words; minimally 3.
	       */

	    d = (size_t*)mem;
	    clearsize = chunksize(p) - (sizeof(size_t));
	    nclears = clearsize / sizeof(size_t);
	    assert(nclears >= 3);

	    if (nclears > 9)
		memset(d, 0, clearsize);

	    else {
		*(d+0) = 0;
		*(d+1) = 0;
		*(d+2) = 0;
		if (nclears > 4) {
		    *(d+3) = 0;
		    *(d+4) = 0;
		    if (nclears > 6) {
			*(d+5) = 0;
			*(d+6) = 0;
			if (nclears > 8) {
			    *(d+7) = 0;
			    *(d+8) = 0;
			}
		    }
		}
	    }
	}
#if 0
	else
	{
	/* Standard unix mmap using /dev/zero clears memory so calloc
	 * doesn't need to actually zero anything....
	 */
	    d = (size_t*)mem;
	    /* Note the additional (sizeof(size_t)) */
	    clearsize = chunksize(p) - 2*(sizeof(size_t));
	    memset(d, 0, clearsize);
	}
#endif
    }
//    __MALLOC_UNLOCK;
    return mem;
}

