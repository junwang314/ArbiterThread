#ifndef _ABSYS_THREAD_CONTROL_H
#define _ABSYS_THREAD_CONTROL_H

#include <linux/pid.h>

#define AB_SET_ME_SPECIAL	0
#define AB_SET_ME_ARBITER	1

#define AB_IDENTITY_SHIFT	1
#define AB_IDENTITY_MASK	(-1 << AB_IDENTITY_SHIFT)   //11...11111110

static inline int is_arbiter_task(struct task_struct *tsk)
{
	return tsk->ab_identity & AB_SET_ME_ARBITER;
}
/*
static inline int is_special_task(struct task_struct *tsk)
{
	return tak->ab_identity & -1 
}
*/

#endif //_ABSYS_THREAD_CONTROL_H
