#ifndef _ABSYS_THREAD_CONTROL_H
#define _ABSYS_THREAD_CONTROL_H

#include <linux/pid.h>
#include <asm/atomic.h>
#include <linux/sched.h>

/* int ab_identity: 
* ===============================|=
*  ab_category (31 bits)     ab_role (1 bit)
*/

#define AB_SET_ME_SPECIAL	0x00000000
#define AB_SET_ME_ARBITER	0x00000001

#define AB_CATEGORY_SHIFT	1
#define AB_ROLE_MASK		0x00000001  

static atomic_t ab_category_count = ATOMIC_INIT(0);

static inline int alloc_category(void)
{
	atomic_inc(&ab_category_count);
	return atomic_read(&ab_category_count); 
}

static inline int get_category(struct task_struct *tsk)
{
	return (tsk->ab_identity >> AB_CATEGORY_SHIFT);
}

static inline int get_role(struct task_struct *tsk)
{
	return (tsk->ab_identity & AB_ROLE_MASK);
}

static inline int is_arbiter(struct task_struct *tsk)
{
	return get_role(tsk);
}
/*
static inline int is_special(struct task_struct *tsk)
{
	return (~get_role(tsk)) & AB_ROLE_MASK;
}
*/
static inline int is_common(struct task_struct *tsk) /* neither arbiter nor special */
{
	return (!get_category(tsk));
}

static inline int is_special(struct task_struct *tsk)
{
	return  (!is_arbiter(tsk) && !is_common(tsk));
}

static inline int convert_to_identity(int ab_category, int ab_role)
{
	return ((ab_category << AB_CATEGORY_SHIFT) | ab_role);
}

static inline struct task_struct *find_my_arbiter(struct task_struct *tsk)
{
	struct task_struct *t;
	list_for_each_entry(t, &tsk->ab_tasks, ab_tasks) {
		if (is_arbiter(t))
			return t;
	}
	return NULL;
}

static inline struct task_struct *get_child_task_by_pid(pid_t pid)
{
	struct task_struct *result;
	rcu_read_lock();
	result = find_task_by_vpid(pid);  // find_task_by_vpid() can be found in kernel/pid.c
	rcu_read_unlock();
	return result;
}

#endif //_ABSYS_THREAD_CONTROL_H
