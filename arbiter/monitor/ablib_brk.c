#include <errno.h>
#include <unistd.h>
//#include <sysdep.h>

//#include <bp-checks.h>

#include <ab_os_interface.h>

/* This must be initialized data because commons can't have aliases.  */
void *__ab_curbrk = 0;

/* Old braindamage in GCC's crtstuff.c requires this symbol in an attempt
   to work around different old braindamage in the old Linux ELF dynamic
   linker.  */
//weak_alias (__ab_curbrk, ___brk_addr)

int __ablib_brk (pid_t pid, void *addr)
{
	void *__unbounded newbrk;

	//INTERNAL_SYSCALL_DECL (err);
	//newbrk = (void *__unbounded) INTERNAL_SYSCALL (absys_brk, err, 1, __ptrvalue(addr));
	newbrk = (void *__unbounded) absys_brk(pid, addr);

	__ab_curbrk = newbrk;
  
	if (newbrk < addr) {
		return -ENOMEM;
	}

	return 0;
}
#define __ablib_brk	ablib_brk
