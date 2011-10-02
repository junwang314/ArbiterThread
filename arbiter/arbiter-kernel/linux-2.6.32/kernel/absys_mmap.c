#include <linux/linkage.h>
#include <linux/kernel.h>
//#include "arbilloc.h"

asmlinkage int sys_absys_mmap(int aaa)
{	
	printk("This is arbiter thread allocator!\n");
	return (aaa+1);
}
