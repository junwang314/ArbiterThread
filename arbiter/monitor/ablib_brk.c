#include <errno.h>
#include <unistd.h>
#include <sysdep.h>

#include <bp-checks.h>

/* This must be initialized data because commons can't have aliases.  */
void *__curbrk = 0;

/* Old braindamage in GCC's crtstuff.c requires this symbol in an attempt
   to work around different old braindamage in the old Linux ELF dynamic
   linker.  */
weak_alias (__curbrk, ___brk_addr)

int __brk (void *addr)
{
  void *__unbounded newbrk;

  INTERNAL_SYSCALL_DECL (err);
  newbrk = (void *__unbounded) INTERNAL_SYSCALL (brk, err, 1, __ptrvalue(addr));


  __curbrk = newbrk;
  
  if (newbrk < addr)
    {
      __set_errno (ENOMEM);
      return -1;
    }

  return 0;
}
weak_alias (__brk, brk)
