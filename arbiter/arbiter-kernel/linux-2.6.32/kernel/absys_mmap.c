#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/sem.h>
#include <linux/mm.h>
#include <linux/mman.h>

#include "absys_mmap.h"
#include "absys_thread_control.h"
#include <linux/abt_common.h>

asmlinkage unsigned long sys_absys_mmap(pid_t childpid, unsigned long addr, 
				unsigned long len, unsigned long prot, 
				unsigned long flags, unsigned long fd, 
				unsigned long pgoff)
{	
	AB_INFO("absys_mmap system call received. argments = (%ld, %ld, %ld, %ld, %ld, %ld, %ld,)\n", (unsigned long) childpid, addr, len, prot, flags, fd, pgoff);

	unsigned long error;
	struct mm_struct *mm;
	struct file *file = NULL;

	/* find task_struct of pid*/
	tsk = xxx(childpid);
	mm = tsk->mm;

	/* check if MAP_ANONYMOUS is assigned: if not, return error */
	flags &= ~(MAP_EXECUTABLE | MAP_DENYWRITE);
	if (!(flags & MAP_ANONYMOUS)) {
		return -EINVAL;		// invalid argument
	}

	/* check if called by ARBITER and do mmap for child thread in its control group */
	if (is_arbiter(current) && (current == find_my_arbiter(tsk))) {
		/* for all the other child threads in the control group, reserve correspongding vma with default access: PROT_NONE */ 
		down_write(&mm->mmap_sem);
		error = do_absys_mmap_pgoff(file, addr, len, prot, flags, pgoff);
		up_write(&mm->mmap_sem);


		down_write(&mm->mmap_sem);
		error = do_absys_mmap_pgoff(file, addr, len, prot, flags, pgoff);
		up_write(&mm->mmap_sem);

	}
	else
		return -EPERM;		// operation not permitted

	return error;
}
