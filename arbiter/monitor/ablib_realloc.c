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


/* ------------------------------ realloc ------------------------------ */
void* ablib_realloc(pid_t pid, void* oldmem, size_t bytes, label_t L)
{
    mstate av;
    ustate unit;

    size_t  nb;              /* padded request size */

    mchunkptr        oldp;            /* chunk corresponding to oldmem */
    size_t  oldsize;         /* its size */

    mchunkptr        newp;            /* chunk to return */
    size_t  newsize;         /* its size */
    void*          newmem;          /* corresponding user mem */

    mchunkptr        next;            /* next contiguous chunk after oldp */

    mchunkptr        remainder;       /* extra space at end of newp */
    unsigned long     remainder_size;  /* its size */

    mchunkptr        bck;             /* misc temp for linking */
    mchunkptr        fwd;             /* misc temp for linking */

    unsigned long     copysize;        /* bytes to copy */
    unsigned int     ncopies;         /* size_t words to copy */
    size_t* s;               /* copy source */
    size_t* d;               /* copy destination */

    void *retval;

    AB_INFO("ablib_realloc called: arguments = (%d, %lx, %d, %lx)\n", pid, (unsigned long)oldmem, bytes, *(unsigned long *)L);

    /* Check for special cases.  */
    if (! oldmem)
	return ablib_malloc(pid, bytes, L);
    if (! bytes) {
	ablib_free (pid, oldmem);
	return NULL;
    }

    __MALLOC_LOCK;
    unit = lookup_ustate_by_mem(oldmem);
    av = unit->unit_av;
    checked_request2size(bytes, nb);

    oldp    = mem2chunk(oldmem);
    oldsize = chunksize(oldp);

    check_inuse_chunk(oldp);

    if (!chunk_is_mmapped(oldp)) {

	if ((unsigned long)(oldsize) >= (unsigned long)(nb)) {
	    /* already big enough; split below */
	    newp = oldp;
	    newsize = oldsize;
	}

	else {
	    next = chunk_at_offset(oldp, oldsize);

	    /* Try to expand forward into top */
	    if (next == unit->unit_top &&
		    (unsigned long)(newsize = oldsize + chunksize(next)) >=
		    (unsigned long)(nb + MINSIZE)) {
		set_head_size(oldp, nb);
		unit->unit_top = chunk_at_offset(oldp, nb);
		set_head(unit->unit_top, (newsize - nb) | PREV_INUSE);
		retval = chunk2mem(oldp);
		__MALLOC_UNLOCK;
		goto DONE;
	    }

	    /* Try to expand forward into next chunk;  split off remainder below */
	    else if (next != unit->unit_top &&
		    !inuse(next) &&
		    (unsigned long)(newsize = oldsize + chunksize(next)) >=
		    (unsigned long)(nb)) {
		newp = oldp;
		unlink(next, bck, fwd);
	    }

	    /* allocate, copy, free */
	    else {
		__MALLOC_UNLOCK;
		newmem = ablib_malloc(pid, nb - MALLOC_ALIGN_MASK, L);
		if (newmem == 0) {
		    retval = 0; /* propagate failure */
		    goto DONE;
		}

		__MALLOC_LOCK;
		newp = mem2chunk(newmem);
		newsize = chunksize(newp);

		/*
		   Avoid copy if newp is next chunk after oldp.
		   */
		if (newp == next) {
		    newsize += oldsize;
		    newp = oldp;
		}
		else {
		    /*
		       Unroll copy of <= 36 bytes (72 if 8byte sizes)
		       We know that contents have an odd number of
		       size_t-sized words; minimally 3.
		       */

		    copysize = oldsize - (sizeof(size_t));
		    s = (size_t*)(oldmem);
		    d = (size_t*)(newmem);
		    ncopies = copysize / sizeof(size_t);
		    assert(ncopies >= 3);

		    if (ncopies > 9)
			memcpy(d, s, copysize);

		    else {
			*(d+0) = *(s+0);
			*(d+1) = *(s+1);
			*(d+2) = *(s+2);
			if (ncopies > 4) {
			    *(d+3) = *(s+3);
			    *(d+4) = *(s+4);
			    if (ncopies > 6) {
				*(d+5) = *(s+5);
				*(d+6) = *(s+6);
				if (ncopies > 8) {
				    *(d+7) = *(s+7);
				    *(d+8) = *(s+8);
				}
			    }
			}
		    }

		    __MALLOC_UNLOCK;
		    ablib_free(pid, oldmem);
		    check_inuse_chunk(newp);
		    retval = chunk2mem(newp);
		    goto DONE;
		}
	    }
	}

	/* If possible, free extra space in old or extended chunk */

	assert((unsigned long)(newsize) >= (unsigned long)(nb));

	remainder_size = newsize - nb;

	if (remainder_size < MINSIZE) { /* not enough extra to split off */
	    set_head_size(newp, newsize);
	    set_inuse_bit_at_offset(newp, newsize);
	    __MALLOC_UNLOCK;
	}
	else { /* split remainder */
	    remainder = chunk_at_offset(newp, nb);
	    set_head_size(newp, nb);
	    set_head(remainder, remainder_size | PREV_INUSE);
	    /* Mark remainder as inuse so free() won't complain */
	    set_inuse_bit_at_offset(remainder, remainder_size);
	    __MALLOC_UNLOCK;
	    ablib_free(pid, chunk2mem(remainder));
	}

	check_inuse_chunk(newp);
	retval = chunk2mem(newp);
	goto DONE;
    }

    /*
       Handle mmap cases
       */

    else {
	size_t offset = oldp->prev_size;
	size_t pagemask = av->pagesize - 1;
	char *cp;
	unsigned long  sum;

	/* Note the extra (sizeof(size_t)) overhead */
	newsize = (nb + offset + (sizeof(size_t)) + pagemask) & ~pagemask;

	/* don't need to remap if still within same page */
	if (oldsize == newsize - offset) {
	    retval = oldmem;
	    __MALLOC_UNLOCK;
	    goto DONE;
	}

	//avoid using mremap()
	//cp = (char*)mremap((char*)oldp - offset, oldsize + offset, newsize, 1);

	//if (cp != (char*)MORECORE_FAILURE) {

	//    newp = (mchunkptr)(cp + offset);
	//    set_head(newp, (newsize - offset)|IS_MMAPPED);

	//    assert(aligned_OK(chunk2mem(newp)));
	//    assert((newp->prev_size == offset));

	//    /* update statistics */
	//    sum = av->mmapped_mem += newsize - oldsize;
	//    if (sum > (unsigned long)(av->max_mmapped_mem))
	//	av->max_mmapped_mem = sum;
	//    sum += av->sbrked_mem;
	//    if (sum > (unsigned long)(av->max_total_mem))
	//	av->max_total_mem = sum;

	//    retval = chunk2mem(newp);
	//    goto DONE;
	//}

	/* Note the extra (sizeof(size_t)) overhead. */
	if ((unsigned long)(oldsize) >= (unsigned long)(nb + (sizeof(size_t)))) {
	    newmem = oldmem; /* do nothing */
	    __MALLOC_UNLOCK;
	}
	else {
	    /* Must alloc, copy, free. */
	    __MALLOC_UNLOCK;
	    newmem = ablib_malloc(pid, nb - MALLOC_ALIGN_MASK, L);
	    if (newmem != 0) {
		memcpy(newmem, oldmem, oldsize - 2*(sizeof(size_t)));
		ablib_free(pid, oldmem);
	    }
	}
	retval = newmem;
    }

 DONE:
    //__MALLOC_UNLOCK;
    /* note: the LOCK in original code is buggy
     * fixed by adding UNLOCK before malloc/free
     */
    return retval;
}

