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
			VM_AB_CONTROL; // controlled VMA indicator
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
			VM_AB_CONTROL; // ontrolled VMA indicator
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
		vma_tmp = find_vma_intersection(mm, addr_tmp, len);
		/* if no intersection found, job done! */
		if (!vma_tmp) 
			break;
		/* find the ummapped area between each existing VMAs in [addr,addr+len] and use mmap_region to map these areas */
		if (vma_tmp->vm_start > addr_tmp && vma_tmp->vm_end < len) {
			len_tmp = vma_tmp->vm_start - addr_tmp;
			addr_ret = propagate_region(tsk, file, addr_tmp, len_tmp, flags, vm_flags, pgoff);
			if (addr_ret < 0) {
				AB_MSG("ERROR in propagate region: absys_propagate_region failed\n");
				return -1;
			}	
		}
		addr_tmp = vma_tmp->vm_end;
		/* has walked through [addr,addr+len], job done! */
		if (addr_tmp > len)
			break;
	}

	return addr;
}

/* system call absys_mmap() */
asmlinkage unsigned long sys_absys_mmap(pid_t childpid, unsigned long addr, 
				unsigned long len, unsigned long prot, 
				unsigned long flags, unsigned long fd, 
				unsigned long pgoff)
{	
	AB_INFO("absys_mmap system call received. argments = (%ld, %ld, %ld, %ld, %ld, %ld, %ld,)\n", (unsigned long) childpid, addr, len, prot, flags, fd, pgoff);

	long ret_target_child, ret_other_child;
	struct task_struct *tsk_target_child, *tsk_other_child;
	struct mm_struct *mm_target_child, *mm_other_child;
	struct file *file = NULL;

	/* check if MAP_ANONYMOUS is assigned: if not, return error */
	flags &= ~(MAP_EXECUTABLE | MAP_DENYWRITE);
	if (!(flags & MAP_ANONYMOUS)) {
		return -EINVAL;		// EINVAL means invalid argument
	}

	/* find target task_struct by pid */
	tsk_target_child = get_child_task_by_pid(childpid);
	AB_INFO("target child pid = %d\n", tsk_target_child->pid);

	/* check if called by ARBITER and then do mmap for child thread in its control group */
	if (is_arbiter(current) && (current == find_my_arbiter(tsk_target_child))) {
		/* for child thread specified by childpid:
		 * set VMA.
		 */
		mm_target_child = tsk_target_child->mm;
		AB_MSG("set VMA for target thread %d:\n", tsk_target_child->pid);
		down_write(&mm_target_child->mmap_sem);
		ret_target_child = do_absys_mmap_pgoff(tsk_target_child, file, addr, len, prot, flags, pgoff);
		up_write(&mm_target_child->mmap_sem);
		AB_MSG("VMA starts @ %lx\n", ret_target_child);
		if (ret_target_child < 0) {
			AB_MSG("ERROR in syscall absys_mmap: do_absys_mmap_pgoff failed\n");
			return -1;
		}

		/* for all the other child threads in the control group:  
		 * 1) go through every child
		 * 2) reserve correspongding VMA with nimimux interference
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
		if (ret_other_child < 0) {
			AB_MSG("ERROR in syscall absys_mmap: do_absys_vma_propagate failed\n");
			return -1;
		}
		}
	}
	else
		return -EPERM;		// EPERM means operation not permitted

	return (unsigned long)ret_target_child;
}

/* system call absys_brk() */
//...







/* specific pf handling for the ab control vm region,
 * the shared page is retrieved from the arbiter thread and
 * stored in the vmf struct
 */
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


//callback functions
static const struct vm_operations_struct ab_vm_ops = {
	.fault                   = abmap_fault,
};
