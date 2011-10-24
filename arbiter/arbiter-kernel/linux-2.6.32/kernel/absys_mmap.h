#ifndef _ABSYS_MMAP_H
#define _ABSYS_MMAP_H

#include <linux/list.h>
#include <linux/sched.h>

/* uncomment if they are defined in mm/mmap.c
extern unsigned long do_absys_mmap_pgoff( struct task_struct, struct file *, 
					  unsigned long, unsigned long, unsigned long,
					  unsigned long, unsigned long);
extern unsigned long do_absys_vma_propagate( struct task_struct, struct file *, 
					     unsigned long, unsigned long, unsigned long,
					     unsigned long, unsigned long);
*/

/* find task_struct by pid */
static inline struct task_struct *get_child_task_by_pid(pid_t pid)
{
	struct task_struct *result;
	rcu_read_lock();
	result = find_task_by_vpid(pid);  // find_task_by_vpid() can be found in kernel/pid.c
	rcu_read_unlock();
	return result;
}


#endif //_ABSYS_MMAP_H
