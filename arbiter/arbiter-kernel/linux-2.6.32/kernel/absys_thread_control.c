#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/sched.h>

#define AB_SET_ME_COMMON	0
#define AB_SET_ME_ARBITER	1

asmlinkage int sys_absys_thread_control(int ab_thread_control_flag)
{
	static int ab_identity_count = 0;
	if (ab_thread_control_flag == AB_SET_ME_ARBITER) {
		/* set ab_identity 
		* with last bit set to 1 to indicate atbiter thread 
		* ab_tasks should be empty 
		*/
		current->ab_identity = 
			((ab_identity_count++) << 1) | 1;
	}
	if (ab_thread_control_flag == AB_SET_ME_COMMON) {
		/* add ab_tasks to the link list */
		list_add_tail(&current->ab_tasks, &current->real_parent->ab_tasks);

		/* ab_identity should be the same as its parent
		* the last bit should be 0 to indicate it is common thread
		*/
		current->ab_identity = current->real_parent->ad_identity && (-1<<1);

	}
	printk("this is greeting from absys_thread_control from kernel!\n");
	return 0;
}
