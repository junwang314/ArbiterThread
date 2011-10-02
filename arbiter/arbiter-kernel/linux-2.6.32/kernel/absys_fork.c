#include <linux/linkage.h>
#include <linux/kernel.h>

asmlinkage int sys_absys_fork(void)
{
	printk("this is greeting from absys_fork from kernel!\n");
	return 0;
}
