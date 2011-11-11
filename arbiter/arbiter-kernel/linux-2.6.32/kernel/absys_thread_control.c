#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/spinlock_types.h>
#include <linux/types.h>
#include <linux/list.h>

#include <linux/absys_thread_control.h>
#include <linux/abt_common.h>

DEFINE_RWLOCK(ab_tasklist_lock);

asmlinkage int sys_absys_thread_control(int ab_role)
{
	int ab_category = 0;
	
	if (ab_role == AB_SET_ME_ARBITER) {
		/* allocate new ab_identity and just leave ab_tasks empty */
		ab_category = alloc_category();
		printk("\nAn arbiter set\n");
	}
	if (ab_role == AB_SET_ME_SPECIAL) {
		/* ab_identity should be the same as its parent */
		ab_category = get_category(current->parent);

		/* add ab_tasks to the doubly linked list */
		write_lock(&ab_tasklist_lock);
		list_add(&current->ab_tasks, &current->parent->ab_tasks);
		write_unlock(&ab_tasklist_lock);

		/* initialize the starting point @ 0x80000000 for ab_heap */
		current->mm->start_ab_brk = AB_HEAP_START;
		current->mm->ab_brk = AB_HEAP_START;

		printk("\nThis is special thread\n");
	}
	/* set ab_identity */
	current->ab_identity = convert_to_identity(ab_category, ab_role);

	printk("the ab_identity is set as %x\n",current->ab_identity);
	printk("list_head ab_tasks: prev-%p current-%p next-%p\n", current->ab_tasks.prev, &current->ab_tasks, current->ab_tasks.next);

	return 0;
}
