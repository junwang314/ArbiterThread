#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/sem.h>
#include <linux/mm.h>
#include <linux/mman.h>

#include "absys_mmap.h"
#include <linux/abt_common.h>

asmlinkage unsigned long sys_absys_mmap(pid_t childpid, unsigned long addr, 
				unsigned long len, unsigned long prot, 
				unsigned long flags, unsigned long fd, 
				unsigned long pgoff)
{	
	AB_INFO("absys_mmap system call received. argments = (%ld, %ld, %ld, %ld, %ld, %ld, %ld,)\n", (unsigned long) childpid, addr, len, prot, flags, fd, pgoff);

	return 0xfeedbeef;

	/* int error = -EBADF; */
	/* struct mm_struct *mm = current->mm; */

	/* down_write(&mm->mmap_sem); */
	/* error = do_mmap_pgoff(NULL, addr, len, prot, flags, pgoff); */
	/* up_write(&mm->mmap_sem); */

	/* return error; */
}
