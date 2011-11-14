#ifndef _ABSYS_MMAP_H
#define _ABSYS_MMAP_H

#include <linux/list.h>

/* uncomment if these two function are moved to mm/mmap.c
extern unsigned long do_absys_mmap_pgoff( struct task_struct, struct file *, 
					  unsigned long, unsigned long, unsigned long,
					  unsigned long, unsigned long);
extern unsigned long do_absys_vma_propagate( struct task_struct, struct file *, 
					     unsigned long, unsigned long, unsigned long,
					     unsigned long, unsigned long);
*/
extern unsigned long propagate_region( struct task_struct *, struct file *,
				unsigned long, unsigned long, unsigned long,
				unsigned int, unsigned long);
extern unsigned long absys_mmap_region( struct task_struct *, struct file *,
				unsigned long, unsigned long, unsigned long,
				unsigned int, unsigned long);
extern unsigned long do_absys_brk(struct task_struct *, unsigned long, unsigned long);
extern unsigned long do_absys_brk_propagate(struct task_struct *, unsigned long, unsigned long);
#endif //_ABSYS_MMAP_H
