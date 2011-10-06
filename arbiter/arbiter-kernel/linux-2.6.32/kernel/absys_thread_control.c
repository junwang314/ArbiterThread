#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <asm/atomic.h>
#include <linux/spinlock.h>
#include <linux/types.h>

#include "absys_thread_control.h"

asmlinkage int sys_absys_thread_control(int ab_thread_control_flag)
{
//	spinlock_t ab_thread_control_lock = SPIN_LOCK_UNLOCKED;
//	static atomic_t ab_identity_count = ATOMIC_INIT(0);
	static int ab_identity_count = 0;

//	spin_lock(&ab_thread_control_lock);	/* critical region starts */
	if (ab_thread_control_flag == AB_SET_ME_ARBITER) {
		/* set ab_identity 
		* with last bit set to 1 to indicate atbiter thread 
		* ab_tasks should be empty 
		*/
//		atomic_inc(&ab_identity_count);
		current->ab_identity = ((ab_identity_count ++) << AB_IDENTITY_SHIFT) | AB_SET_ME_ARBITER;
		printk("\nAn arbiter set\n");
	}
	if (ab_thread_control_flag == AB_SET_ME_SPECIAL) {
		/* add ab_tasks to the link list */
		list_add_tail(&current->ab_tasks, &current->parent->ab_tasks);

		/* ab_identity should be the same as its parent
		* the last bit should be 0 to indicate it is common thread
		*/
		current->ab_identity = current->parent->ab_identity & AB_IDENTITY_MASK;
		printk("\nThis is special thread\n");

	}
	printk("the ab_identity is set as %x\n",current->ab_identity);
	printk("the prev and next pointer in ab_task: %p %p\n", current->ab_tasks.prev, current->ab_tasks.next);
//	spin_unlock(ab_thread_control_lock);	/* critical region ends */

	return 0;
}
