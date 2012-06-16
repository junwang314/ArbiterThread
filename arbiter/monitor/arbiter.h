#ifndef _ARBITER_H
#define _ARBITER_H

#include <stddef.h>
#include <lib/linked_list.h>
#include <lib/timer.h>

/* data structure that describes the run-time  */
/* state of an arbiter thread */

struct arbiter_thread {
	//unix domain socket for client connections
	int monitor_sock;
	
	//list for active clients
	struct linked_list client_list;
};

extern struct arbiter_thread arbiter;

#ifdef _SYSCALL_COUNT_TIME
enum syscall_code {
	SETME,
	SBRK,
	MMAP,
	MPROTECT
};
extern int syscall_count[4];
extern uint64_t syscall_time[4];
#endif

#endif  //_ARBITER_H
