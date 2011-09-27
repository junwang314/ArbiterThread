#include <linux/linkage.h>
#include <linux/kernel.h>
#include "arbilloc.h"

asmlinkage int sys_arbilloc()
{
	printfk(KERN_EMERG "This is arbiter thread allocator!\n");
	return RET;
}
