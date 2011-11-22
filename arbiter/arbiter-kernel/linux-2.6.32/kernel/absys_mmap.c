#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/sem.h>

#include <linux/slab.h>
#include <linux/backing-dev.h>
#include <linux/mm.h>
#include <linux/shm.h>
#include <linux/mman.h>
#include <linux/pagemap.h>
#include <linux/swap.h>
#include <linux/syscalls.h>
#include <linux/capability.h>
#include <linux/init.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/personality.h>
#include <linux/security.h>
#include <linux/ima.h>
#include <linux/hugetlb.h>
#include <linux/profile.h>
#include <linux/module.h>
#include <linux/mount.h>
#include <linux/mempolicy.h>
#include <linux/rmap.h>
#include <linux/mmu_notifier.h>
#include <linux/perf_event.h>

#include <asm/uaccess.h>
#include <asm/cacheflush.h>
#include <asm/tlb.h>
#include <asm/mmu_context.h>

#include "absys_mmap.h"
#include <linux/absys_thread_control.h>
#include <linux/abt_common.h>

#ifndef arch_mmap_check	  // these three lines are copied from mm/mmap.c to declare arch_mmap_check
#define arch_mmap_check(addr, len, flags)	(0)
#endif

/* do_absys_mmap_pgoff()
 * The caller must hold down_write(&tsk->mm->mmap_sem).
 */
unsigned long do_absys_mmap_pgoff( struct task_struct *tsk, struct file *file, 
				   unsigned long addr, unsigned long len, unsigned long prot,
				   unsigned long flags, unsigned long pgoff)
{
	struct mm_struct * mm = tsk->mm;
	struct inode *inode;
	unsigned int vm_flags;
	int error;
	unsigned long reqprot = prot;

	/*
	 * Does the application expect PROT_READ to imply PROT_EXEC?
	 *
	 * (the exception is when the underlying filesystem is noexec
	 *  mounted, in which case we dont add PROT_EXEC.)
	 */
	if ((prot & PROT_READ) && (tsk->personality & READ_IMPLIES_EXEC))
		if (!(file && (file->f_path.mnt->mnt_flags & MNT_NOEXEC)))
			prot |= PROT_EXEC;

	if (!len)
		return -EINVAL;

	if (!(flags & MAP_FIXED))
		addr = round_hint_to_min(addr);

	error = arch_mmap_check(addr, len, flags);
	if (error)
		return error;

	/* Careful about overflows.. */
	len = PAGE_ALIGN(len);
	if (!len || len > TASK_SIZE)
		return -ENOMEM;

	/* offset overflow? */
	if ((pgoff + (len >> PAGE_SHIFT)) < pgoff)
               return -EOVERFLOW;

	/* Too many mappings? */
	if (mm->map_count > sysctl_max_map_count)
		return -ENOMEM;

	if (flags & MAP_HUGETLB) {
		struct user_struct *user = NULL;
		if (file)
			return -EINVAL;

		/*
		 * VM_NORESERVE is used because the reservations will be
		 * taken when vm_ops->mmap() is called
		 * A dummy user value is used because we are not locking
		 * memory so no accounting is necessary
		 */
		len = ALIGN(len, huge_page_size(&default_hstate));
		file = hugetlb_file_setup(HUGETLB_ANON_FILE, len, VM_NORESERVE,
						&user, HUGETLB_ANONHUGE_INODE);
		if (IS_ERR(file))
			return PTR_ERR(file);
	}

	/* Obtain the address to map to. we verify (or select) it and ensure
	 * that it represents a valid section of the address space.
	 */
	addr = get_unmapped_area(file, addr, len, pgoff, flags);
	AB_DBG("in do_absys_mmap_pgoff: get_unmmapped_area returns %lx\n", addr);
	if (addr & ~PAGE_MASK)
		return addr;

	/* Do simple checking here so the lower-level routines won't have
	 * to. we assume access permissions have been handled by the open
	 * of the memory object, so we don't do any here.
	 */
	vm_flags = calc_vm_prot_bits(prot) | calc_vm_flag_bits(flags) |
			mm->def_flags | VM_MAYREAD | VM_MAYWRITE | VM_MAYEXEC |
			VM_AB_CONTROL | VM_SHARED; // controlled VMA indicator
	AB_INFO("do_absys_mmap_pgoff: vm_flags = %x\n", vm_flags);

	if (flags & MAP_LOCKED)
		if (!can_do_mlock())
			return -EPERM;

	/* mlock MCL_FUTURE? */
	if (vm_flags & VM_LOCKED) {
		unsigned long locked, lock_limit;
		locked = len >> PAGE_SHIFT;
		locked += mm->locked_vm;
		lock_limit = tsk->signal->rlim[RLIMIT_MEMLOCK].rlim_cur;
		lock_limit >>= PAGE_SHIFT;
		if (locked > lock_limit && !capable(CAP_IPC_LOCK))
			return -EAGAIN;
	}

	inode = file ? file->f_path.dentry->d_inode : NULL;

	if (file) {
		switch (flags & MAP_TYPE) {
		case MAP_SHARED:
			if ((prot&PROT_WRITE) && !(file->f_mode&FMODE_WRITE))
				return -EACCES;

			/*
			 * Make sure we don't allow writing to an append-only
			 * file..
			 */
			if (IS_APPEND(inode) && (file->f_mode & FMODE_WRITE))
				return -EACCES;

			/*
			 * Make sure there are no mandatory locks on the file.
			 */
			if (locks_verify_locked(inode))
				return -EAGAIN;

			vm_flags |= VM_SHARED | VM_MAYSHARE;
			if (!(file->f_mode & FMODE_WRITE))
				vm_flags &= ~(VM_MAYWRITE | VM_SHARED);

			/* fall through */
		case MAP_PRIVATE:
			if (!(file->f_mode & FMODE_READ))
				return -EACCES;
			if (file->f_path.mnt->mnt_flags & MNT_NOEXEC) {
				if (vm_flags & VM_EXEC)
					return -EPERM;
				vm_flags &= ~VM_MAYEXEC;
			}

			if (!file->f_op || !file->f_op->mmap)
				return -ENODEV;
			break;

		default:
			return -EINVAL;
		}
	} else {
		switch (flags & MAP_TYPE) {
		case MAP_SHARED:
			/*
			 * Ignore pgoff.
			 */
			pgoff = 0;
			vm_flags |= VM_SHARED | VM_MAYSHARE;
			break;
		case MAP_PRIVATE:
			/*
			 * Set pgoff according to addr for anon_vma.
			 */
			pgoff = addr >> PAGE_SHIFT;
			break;
		default:
			return -EINVAL;
		}
	}

	error = security_file_mmap(file, reqprot, prot, flags, addr, 0);
	if (error)
		return error;
	error = ima_file_mmap(file, prot);
	if (error)
		return error;

	return absys_mmap_region(tsk, file, addr, len, flags, vm_flags, pgoff);
}

/* do_absys_vma_propagate()
 * The caller must hold down_write(&tsk->mm->mmap_sem).
 */
unsigned long do_absys_vma_propagate( struct task_struct *tsk, struct file *file, 
				      unsigned long addr, unsigned long len, unsigned long prot,
				      unsigned long flags, unsigned long pgoff)
{
	struct mm_struct * mm = tsk->mm;
	struct inode *inode;
	unsigned int vm_flags;
	int error;
	unsigned long reqprot = prot;
	/* temporary variable used for doing minimum interference, at the end of this function */
	unsigned long addr_tmp, len_tmp, addr_ret;
	struct vm_area_struct *vma_tmp;

	/*
	 * Does the application expect PROT_READ to imply PROT_EXEC?
	 *
	 * (the exception is when the underlying filesystem is noexec
	 *  mounted, in which case we dont add PROT_EXEC.)
	 */
	if ((prot & PROT_READ) && (tsk->personality & READ_IMPLIES_EXEC))
		if (!(file && (file->f_path.mnt->mnt_flags & MNT_NOEXEC)))
			prot |= PROT_EXEC;

	if (!len)
		return -EINVAL;
/* these two lines are unnecessary because MAP_FIXED is forced to set
	if (!(flags & MAP_FIXED))
		addr = round_hint_to_min(addr);
*/

/* the following lines are unnecessary 
 * because these has been done by do_absys_mmap_pgoff() in sys_absys_mmap()
 */ 
//	error = arch_mmap_check(addr, len, flags);
//	if (error)
//		return error;
//
//	/* Careful about overflows.. */
//	len = PAGE_ALIGN(len);
//	if (!len || len > TASK_SIZE)
//		return -ENOMEM;
//
//	/* offset overflow? */
//	if ((pgoff + (len >> PAGE_SHIFT)) < pgoff)
//              return -EOVERFLOW;
//
//	/* Too many mappings? */
//	if (mm->map_count > sysctl_max_map_count)
//		return -ENOMEM;
//
//	if (flags & MAP_HUGETLB) {
//		struct user_struct *user = NULL;
//		if (file)
//			return -EINVAL;
//
		/*
		 * VM_NORESERVE is used because the reservations will be
		 * taken when vm_ops->mmap() is called
		 * A dummy user value is used because we are not locking
		 * memory so no accounting is necessary
		 */
//		len = ALIGN(len, huge_page_size(&default_hstate));
//		file = hugetlb_file_setup(HUGETLB_ANON_FILE, len, VM_NORESERVE,
//						&user, HUGETLB_ANONHUGE_INODE);
//		if (IS_ERR(file))
//			return PTR_ERR(file);
//	}

	/* Obtain the address to map to. we verify (or select) it and ensure
	 * that it represents a valid section of the address space.
	 */
/* the three lines are unnecessary because we want keep addr unchanged 
	addr = get_unmapped_area(file, addr, len, pgoff, flags);
	if (addr & ~PAGE_MASK)
		return addr;
*/
	/* Do simple checking here so the lower-level routines won't have
	 * to. we assume access permissions have been handled by the open
	 * of the memory object, so we don't do any here.
	 */
	vm_flags = calc_vm_prot_bits(prot) | calc_vm_flag_bits(flags) |
			mm->def_flags | VM_MAYREAD | VM_MAYWRITE | VM_MAYEXEC |
			VM_AB_CONTROL | VM_SHARED; // ontrolled VMA indicator
	vm_flags = vm_flags & 0xfffffff0; // none access as default

	AB_INFO("do_absys_vma_propagate: vm_flags = %x\n", vm_flags);

	if (flags & MAP_LOCKED)
		if (!can_do_mlock())
			return -EPERM;

	/* mlock MCL_FUTURE? */
	if (vm_flags & VM_LOCKED) {
		unsigned long locked, lock_limit;
		locked = len >> PAGE_SHIFT;
		locked += mm->locked_vm;
		lock_limit = tsk->signal->rlim[RLIMIT_MEMLOCK].rlim_cur;
		lock_limit >>= PAGE_SHIFT;
		if (locked > lock_limit && !capable(CAP_IPC_LOCK))
			return -EAGAIN;
	}

	inode = file ? file->f_path.dentry->d_inode : NULL;

	if (file) {
		switch (flags & MAP_TYPE) {
		case MAP_SHARED:
			if ((prot&PROT_WRITE) && !(file->f_mode&FMODE_WRITE))
				return -EACCES;

			/*
			 * Make sure we don't allow writing to an append-only
			 * file..
			 */
			if (IS_APPEND(inode) && (file->f_mode & FMODE_WRITE))
				return -EACCES;

			/*
			 * Make sure there are no mandatory locks on the file.
			 */
			if (locks_verify_locked(inode))
				return -EAGAIN;

			vm_flags |= VM_SHARED | VM_MAYSHARE;
			if (!(file->f_mode & FMODE_WRITE))
				vm_flags &= ~(VM_MAYWRITE | VM_SHARED);

			/* fall through */
		case MAP_PRIVATE:
			if (!(file->f_mode & FMODE_READ))
				return -EACCES;
			if (file->f_path.mnt->mnt_flags & MNT_NOEXEC) {
				if (vm_flags & VM_EXEC)
					return -EPERM;
				vm_flags &= ~VM_MAYEXEC;
			}

			if (!file->f_op || !file->f_op->mmap)
				return -ENODEV;
			break;

		default:
			return -EINVAL;
		}
	} else {
		switch (flags & MAP_TYPE) {
		case MAP_SHARED:
			/*
			 * Ignore pgoff.
			 */
			pgoff = 0;
			vm_flags |= VM_SHARED | VM_MAYSHARE;
			break;
		case MAP_PRIVATE:
			/*
			 * Set pgoff according to addr for anon_vma.
			 */
			pgoff = addr >> PAGE_SHIFT;
			break;
		default:
			return -EINVAL;
		}
	}

	error = security_file_mmap(file, reqprot, prot, flags, addr, 0);
	if (error)
		return error;
	error = ima_file_mmap(file, prot);
	if (error)
		return error;

	/* doing minimum interference */
	for (addr_tmp = addr; ; ) {		
		/* find the first VMA that overlaps the given interval */
		vma_tmp = find_vma_intersection(mm, addr_tmp, addr + len);
		/* if no intersection found, job done! */
		if (!vma_tmp) {
			break;
		}
		/* find the ummapped area between each existing VMAs in [addr,addr+len] and use mmap_region to map these areas */
		if (vma_tmp->vm_start > addr_tmp && vma_tmp->vm_start < addr + len) {
			len_tmp = vma_tmp->vm_start - addr_tmp;
			addr_ret = propagate_region(tsk, file, addr_tmp, len_tmp, flags, vm_flags, pgoff);
			AB_MSG("Unexpected event in syscall absys_mmap: there are VMA intersections in child %d\n", tsk->pid);
		}	
		addr_tmp = vma_tmp->vm_end;
		/* has walked through [addr,addr+len], job done! */
		if (addr_tmp >= addr + len)
			break;
	}
	/* has walked through [addr,addr+len] and mapped all the holes */
	if (addr_tmp >= addr + len) {
		return addr;
	}	
	/* otherwise, no intersections found, therefore map all [addr,addr+len] */
	return propagate_region(tsk, file, addr, len, flags, vm_flags, pgoff);
}

/* system call absys_mmap() */
asmlinkage unsigned long sys_absys_mmap(pid_t childpid, unsigned long addr, 
				unsigned long len, unsigned long prot, 
				unsigned long flags, unsigned long fd, 
				unsigned long pgoff)
{	
	
	unsigned long ret_target_child, ret_other_child;
	struct task_struct *tsk_target_child, *tsk_other_child;
	struct mm_struct *mm_target_child, *mm_other_child;
	struct file *file = NULL;

	AB_INFO("absys_mmap system call received. argments = (%ld, %ld, %ld, %ld, %ld, %ld, %ld,)\n", (unsigned long) childpid, addr, len, prot, flags, fd, pgoff);

	/* check if MAP_ANONYMOUS is assigned: if not, return error */
	flags &= ~(MAP_EXECUTABLE | MAP_DENYWRITE);
	if (!(flags & MAP_ANONYMOUS)) {
		return -EINVAL;		// EINVAL means invalid argument
	}

	/* find target task_struct by pid */
	tsk_target_child = get_child_task_by_pid(childpid);
	AB_INFO("target child pid = %d\n", tsk_target_child->pid);

	/* check if called by ARBITER and then do mmap for child thread in its control group */
	if (!is_arbiter(current) || current != find_my_arbiter(tsk_target_child)) {
		return -EPERM;		// EPERM means operation not permitted
	}	

	/* for child thread specified by childpid:
	 * set VMA.
	 */
	mm_target_child = tsk_target_child->mm;
	AB_MSG("set VMA for target thread %d:\n", tsk_target_child->pid);
	down_write(&mm_target_child->mmap_sem);
	ret_target_child = do_absys_mmap_pgoff(tsk_target_child, file, addr, len, prot, flags, pgoff);
	up_write(&mm_target_child->mmap_sem);
	AB_MSG("VMA starts @ %lx\n", ret_target_child);
	//if (ret_target_child < 0) {
	//	AB_MSG("ERROR in syscall absys_mmap: do_absys_mmap_pgoff failed: %d\n", ret_target_child);
	//	return -1;
	//}

	/* for all the other child threads in the control group:  
	 * 1) go through every child
	 * 2) reserve correspongding VMA with minimum interference
	 */ 
	list_for_each_entry(tsk_other_child, &tsk_target_child->ab_tasks, ab_tasks) {
		/* if it is arbiter thread or target child thread, continue to the next loop */
		if (tsk_other_child == current || tsk_other_child == tsk_target_child) {
			continue;
		}
		mm_other_child = tsk_other_child->mm;
		AB_MSG("propagate VMA for child thread %d:\n", tsk_other_child->pid);
		down_write(&mm_other_child->mmap_sem);
		ret_other_child = do_absys_vma_propagate(tsk_other_child, file, ret_target_child, len, prot, flags, pgoff); // use ret_target_child instead of addr
		up_write(&mm_other_child->mmap_sem);
		AB_MSG("VMA starts @ %lx\n", ret_other_child);
		//if (ret_other_child < 0) {
		//	AB_MSG("ERROR in syscall absys_mmap: do_absys_vma_propagate failed: %d\n", ret_other_child);
		//	return -1;
		//}
	}
	return (unsigned long)ret_target_child;
}

/* system call absys_brk() */
asmlinkage unsigned long sys_absys_brk(pid_t childpid, unsigned long ab_brk)
{
	unsigned long rlim, retval;
	unsigned long newbrk, oldbrk;
	struct task_struct *tsk;
	struct mm_struct *mm;
	unsigned long min_brk;
	
	AB_INFO("absys_brk system call received. argments = (%ld, %ld,)\n", (unsigned long) childpid, ab_brk);

	/* Although childpid is an argument, it is only used to look up for the ab_brk (as it is the same for all child threads) */
	tsk = get_child_task_by_pid(childpid);
	AB_INFO("target child pid = %d\n", tsk->pid);

	/* Check if called by ARBITER and then do mmap for child thread in its control group */
	if (!is_arbiter(current) || current != find_my_arbiter(tsk)) {
		return -EPERM;		// EPERM means operation not permitted
	}	

	/* Do some preliminary check */
	mm = tsk->mm;
	down_write(&mm->mmap_sem);
	/* unnecassary code from brk()
	#ifdef CONFIG_COMPAT_BRK
		min_brk = mm->end_code;
	#else
		min_brk = mm->start_brk;
	#endif
	*/	
	min_brk = mm->start_ab_brk;
	if (ab_brk < min_brk)
		goto out;
/* it seems that we do not need the following code, which check the against the maximum heap size */
	/* TODO: check if these are really not needed
	 * Check against rlimit here. If this check is done later after the test
	 * of oldbrk with newbrk then it can escape the test and let the data
	 * segment grow beyond its set limit the in case where the limit is
	 * not page aligned -Ram Gupta
	 */
/*	rlim = current->signal->rlim[RLIMIT_DATA].rlim_cur;
	if (rlim < RLIM_INFINITY && (ab_brk - mm->start_ab_brk) +
			(mm->end_data - mm->start_data) > rlim)
		goto out;
*/
	newbrk = PAGE_ALIGN(ab_brk);
	AB_DBG("ab_brk = %lx, newbrk = %lx\n", ab_brk, newbrk);
	oldbrk = PAGE_ALIGN(mm->ab_brk);
	AB_DBG("mm->ab_brk = %lx, oldbrk = %lx\n", mm->ab_brk, oldbrk);
	if (oldbrk == newbrk) {
		up_write(&mm->mmap_sem);
		goto set_ab_brk;
	}

	/* TODO: Always allow shrinking ab_brk.
	if (ab_brk <= mm->ab_brk) {
		if (!do_munmap(mm, newbrk, oldbrk-newbrk))
			goto set_ab_brk;
		goto out;
	}*/

	/* Check against existing mmap mappings, simply return if existing VMAs are in the way. */
	up_write(&mm->mmap_sem);
	list_for_each_entry(tsk, &tsk->ab_tasks, ab_tasks) {
		/* if it is arbiter thread, continue to the next loop */
		if(tsk == current) {
			continue;
		}
		mm = tsk->mm;
		down_write(&mm->mmap_sem);
		if (find_vma_intersection(mm, oldbrk, newbrk+PAGE_SIZE)) {
			AB_MSG("Exit from absys_brk when checking VMA intersection for child %d\n", tsk->pid);
			goto out;
		}
		up_write(&mm->mmap_sem);
	}
	// the following code is removed from previous version
	//for (tmpbrk = oldbrk; ; ) {
		/* find the first VMA that overlaps the given interval */
	//	vma_tmp = find_vma_intersection(mm, tmpbrk, newbrk+PAGE_SIZE);
		/* if no intersection found, job done! */
	//	if (!vma_tmp) {
	//		break;
	//	}
		/* if there is any system VMA, goto out */
	//	if (vma_tmp && ((vma_tmp->vm_flags & VM_AB_CONTROL) == 0)) {
	//		goto out;
	//	}
	//	tmpbrk = vma_tmp->vm_end;
		/* has walked through [oldbrk, newbrk], job done! */
	//	if (tmpbrk >= newbrk+PAGE_SIZE) {
	//		break;
	//	}
	//}

	/* Ok, looks good - let it rip. */
	list_for_each_entry(tsk, &tsk->ab_tasks, ab_tasks) {
		/* if it is arbiter thread, continue to the next loop */
		if(tsk == current) {
			continue;
		}
		mm = tsk->mm;
		down_write(&mm->mmap_sem);
		if (find_vma_intersection(mm, oldbrk, newbrk+PAGE_SIZE)) {
			AB_MSG("Unexpected event in absys_brk: cannot increase ab_brk due to existing VMA in child %d\n", tsk->pid);
			goto out;
		}
		if (do_absys_brk(tsk, oldbrk, newbrk-oldbrk) != oldbrk) {
			AB_MSG("ERROR in absys_brk: error in do_absys_brk for child %d\n", tsk->pid);
			goto out;
		}
		up_write(&mm->mmap_sem);
	}
			 
set_ab_brk:
	list_for_each_entry(tsk, &tsk->ab_tasks, ab_tasks) {
		/* if it is arbiter thread, continue to the next loop */
		if(tsk == current) {
			continue;
		}
		mm = tsk->mm;
		down_write(&mm->mmap_sem);
		mm->ab_brk = ab_brk;
		up_write(&mm->mmap_sem);
	}
	retval = mm->ab_brk;
	return retval;

/* Unexpected event if getting here*/
out:
	AB_MSG("Unexpected event in absys_brk!\n"); 
	retval = mm->ab_brk;
	up_write(&mm->mmap_sem);
	return retval;
}

/* system call absys_munmap() */
asmlinkage int sys_absys_munmap(pid_t childpid, unsigned long addr, size_t length)
{
	int ret_target_child, ret_other_child;
	//long ret_target_child, ret_other_child;
	struct task_struct *tsk_target_child, *tsk_other_child;
	struct mm_struct *mm_target_child, *mm_other_child;
	unsigned long addr_tmp;
	struct vm_area_struct *vma_tmp;

	AB_INFO("absys_munmap system call received. argments = (%ld, %ld, %d,)\n", (unsigned long) childpid, addr, length);
	
	/* find target task_struct by pid */
	tsk_target_child = get_child_task_by_pid(childpid);
	AB_INFO("target child pid = %d\n", tsk_target_child->pid);

	/* check if called by ARBITER and then do munmap for child thread in its control group */
	if (!is_arbiter(current) || current != find_my_arbiter(tsk_target_child)) {
		return -EPERM;		// EPERM means operation not permitted
	}	
	
	profile_munmap(addr);		// TODO: what is this?

	/* for child thread specified by childpid:
	 * unmap both system VMA and abt VMA 
	 */
	mm_target_child = tsk_target_child->mm;
	down_write(&mm_target_child->mmap_sem);
	ret_target_child = do_munmap(mm_target_child, addr, length);
	up_write(&mm_target_child->mmap_sem);
	if (ret_target_child < 0) {
		AB_MSG("ERROR in syscall absys_mummap: do_munmap failed for target child %d\n", tsk_target_child->pid);
		return ret_target_child;
	}

	/* for all the other child threads in the control group:  
	 * unmap abt VMA, leave system VMA unchanged (minimum interference)
	 */ 
	list_for_each_entry(tsk_other_child, &tsk_target_child->ab_tasks, ab_tasks) {
		/* if it is arbiter thread or target child thread, continue to the next loop */
		if (tsk_other_child == current || tsk_other_child == tsk_target_child) {
			continue;
		}
		mm_other_child = tsk_other_child->mm;
		down_write(&mm_other_child->mmap_sem);
		// TODO: check existing vma
		for (addr_tmp = addr; ; ) {
			/* find the first VMA that overlaps the given interval */
			vma_tmp = find_vma_intersection(mm_other_child, addr_tmp, addr + length);
			/* if no intersection found, job done! */
			if (!vma_tmp) {
				ret_other_child = 0;
				break;
			}
			/* if there is any abt VMA, unmap it */
			if ((vma_tmp->vm_start < addr + length) && (vma_tmp->vm_flags & VM_AB_CONTROL)) {
				ret_other_child = do_munmap(vma_tmp->vm_mm, vma_tmp->vm_start, (vma_tmp->vm_end - vma_tmp->vm_start));
			}
			/* if there is any system VMA, skip it. but this is unexpected event. */
			if ((vma_tmp->vm_start < addr + length) && (vma_tmp->vm_flags & VM_AB_CONTROL) == 0) {
				AB_MSG("Unexpected event in syscall absys_munmap: there are VMA intersections in child %d\n", tsk_other_child->pid);
			}
			addr_tmp = vma_tmp->vm_end;
			/* has walked through [addr, addr+length], job done! */
			if (addr_tmp >= addr + length) {
				break;
			}
		}
		up_write(&mm_other_child->mmap_sem);
		if (ret_other_child < 0) {
			AB_MSG("ERROR in syscall absys_munmap: do_munmap failed for child %d\n", tsk_other_child->pid);
			return ret_other_child;
		}
	}
	return 0;
}


/* specific pf handling for the ab control vm region,
 * the shared page is retrieved from the arbiter thread and
 * stored in the vmf struct
 */
struct page *abmap_get_page(struct vm_area_struct *vma, unsigned long address)
{
	int ret = 0;
	struct task_struct *tsk = current;
	struct page *page = NULL;
	struct task_struct *abt;
	struct mm_struct *mm;

	//the faulting task must be a child task in a control group
	ab_assert(is_special(tsk));
	
	//retrive the arbiter thread
	abt = find_my_arbiter(tsk);
	ab_assert(abt);

	mm = abt->mm;
	ab_assert(mm);

	//hold the mmap read lock of the abthread
	down_read(&mm->mmap_sem);
	//get the exact page from the abt thread
	ret = get_user_pages(abt, mm, 
			     (unsigned  long)address, 
			     1,                     //only one page
			     1,                     //TODO: not sure about this flag  
			     0, 
			     &page,
			     NULL);

	if (ret != 1 || !page) {
		up_read(&mm->mmap_sem);
		AB_MSG("abmap_fault: the request page is not found in arbiter's address space! faulting child pid = %lu, abiter_pid = %lu, address = %lx, page = %lx, ret = %d.\n",
		       (unsigned long)tsk->pid,
		       (unsigned long)abt->pid,
		       (unsigned long)address,
		       (unsigned long)page,
		       ret);
		return NULL;
	}

	up_read(&mm->mmap_sem);

	AB_INFO("abmap_fault: found the request page in arbiter's address space! faulting child pid = %lu, abiter_pid = %lu, address = %lx, page = %lx, ret = %d.\n",
		(unsigned long)tsk->pid,
		(unsigned long)abt->pid,
		(unsigned long)address,
		(unsigned long)page,
		ret);

	return page;
	
}




/* specific pf handling for the ab control vm region,
 * the shared page is retrieved from the arbiter thread and
 * stored in the vmf struct
 */
/* replaced by the above abmap_get_page()
int abmap_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	int ret = 0;
	struct task_struct *tsk = current;
	struct page *page = NULL;
	struct task_struct *abt;
	struct mm_struct *mm;

	//the faulting task must be a child task in a control group
	ab_assert(is_special(tsk));
	
	//retrive the arbiter thread
	abt = find_my_arbiter(tsk);
	ab_assert(abt);

	mm = abt->mm;
	ab_assert(mm);

	//hold the mmap read lock of the abthread
	down_read(&mm->mmap_sem);
	//get the exact page from the abt thread
	ret = get_user_pages(abt, mm, 
			     (unsigned  long)vmf->virtual_address, 
			     1,                     //only one page
			     1,                     //TODO: not sure about this flag  
			     0, 
			     &page,
			     NULL);

	if (ret != 1 || !page) {
		up_read(&mm->mmap_sem);
		AB_MSG("abmap_fault: the request page is not found in arbiter's address space! faulting child pid = %lu, abiter_pid = %lu, address = %lx, page = %lx, ret = %d.\n",
		       (unsigned long)tsk->pid,
		       (unsigned long)abt->pid,
		       (unsigned long)vmf->virtual_address,
		       (unsigned long)page,
		       ret);
		return VM_FAULT_NOPAGE;
	}

	up_read(&mm->mmap_sem);

	AB_INFO("abmap_fault: found the request page in arbiter's address space! faulting child pid = %lu, abiter_pid = %lu, address = %lx, page = %lx, ret = %d.\n",
		(unsigned long)tsk->pid,
		(unsigned long)abt->pid,
		(unsigned long)vmf->virtual_address,
		(unsigned long)page,
		ret);

	vmf->page = page;
	//the caller will enforce the page to be locked.
	return ret;
}
*/

//callback functions
const struct vm_operations_struct ab_vm_ops = {
	//.fault                   = abmap_fault,
	.fault                   = NULL,
};
