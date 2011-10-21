#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/sem.h>
#include <linux/mm.h>
#include <linux/mman.h>

#include "absys_mmap.h"

asmlinkage long sys_absys_mmap(	unsigned long addr, unsigned long len, 
				unsigned long prot, unsigned long flags, 
				unsigned long fd, unsigned long pgoff)
{	
	int error = -EBADF;
	struct mm_struct *mm = current->mm;

	down_write(&mm->mmap_sem);
	error = do_mmap_pgoff(NULL, addr, len, prot, flags, pgoff);
	up_write(&mm->mmap_sem);

	return error;;
}
