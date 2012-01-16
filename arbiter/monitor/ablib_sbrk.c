#include <errno.h>
#include <stdint.h>
#include <unistd.h>

/* Defined in ablib_brk.c.  */
extern void *__curbrk;
extern int __brk (void *addr);

/* Defined in init-first.c.  */
extern int __libc_multiple_libcs attribute_hidden;

/* Extend the process's data space by INCREMENT.
   If INCREMENT is negative, shrink data space by - INCREMENT.
   Return start of new space allocated, or -1 for errors.  */
void *
__sbrk (intptr_t increment)
{
  void *oldbrk;

  /* If this is not part of the dynamic library or the library is used
     via dynamic loading in a statically linked program update
     __curbrk from the kernel's brk value.  That way two separate
     instances of __brk and __sbrk can share the heap, returning
     interleaved pieces of it.  */
  if (__curbrk == NULL || __libc_multiple_libcs)
    if (__brk (0) < 0)          /* Initialize the break.  */
      return (void *) -1;

  if (increment == 0)
    return __curbrk;

  oldbrk = __curbrk;
  if ((increment > 0
       ? ((uintptr_t) oldbrk + (uintptr_t) increment < (uintptr_t) oldbrk)
       : ((uintptr_t) oldbrk < (uintptr_t) -increment))
      || __brk (oldbrk + increment) < 0)
    return (void *) -1;

  return oldbrk;
}
libc_hidden_def (__sbrk)
weak_alias (__sbrk, sbrk)

