#include <errno.h>
#include <stdint.h>
#include <unistd.h>

#include "ablib_malloc.h"

/* Defined in ablib_brk.c.  */
extern void *__ab_curbrk;
extern int __ablib_brk (pid_t pid, void *addr);

/* Defined in init-first.c.  */
extern int __libc_multiple_libcs attribute_hidden;

#define __ablib_sbrk	ablib_sbrk

/* Extend the process's data space by INCREMENT.
   If INCREMENT is negative, shrink data space by - INCREMENT.
   Return start of new space allocated, or -1 for errors.  */
void * __ablib_sbrk (pid_t pid, intptr_t increment)
{
	void *oldbrk;

	//make sure increment is UNIT_SIZE
	assert(increment == UNIT_SIZE);
  
	/* If this is not part of the dynamic library or the library is used
	via dynamic loading in a statically linked program update
	__curbrk from the kernel's brk value.  That way two separate
	instances of __brk and __sbrk can share the heap, returning
	interleaved pieces of it.  */
	if (__ab_curbrk == NULL) {
		if (__ablib_brk (pid, 0) < 0)          /* Initialize the break.  */
			return (void *) -1;
		assert(__ablib_brk(pid, 0) == 0x8000000);
	}

	if (increment == 0)
		return __ab_curbrk;

	oldbrk = __ab_curbrk;
	if ((increment > 0
		? ((uintptr_t) oldbrk + (uintptr_t) increment < (uintptr_t) oldbrk)
		: ((uintptr_t) oldbrk < (uintptr_t) -increment))
		|| __ablib_brk (pid, oldbrk + increment) < 0)
		return (void *) -1;

	return oldbrk;
}
//libc_hidden_def (__ablib_sbrk)


