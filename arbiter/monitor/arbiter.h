#ifndef _ARBITER_H
#define _ARBITER_H


/* data structure that describes the run-time  */
/* state of an arbiter thread */

struct arbiter_thread {
	//unix domain socket for client connections
	int monitor_sock;

	//More to add... e.g., child thread table...
};


#endif  //_ARBITER_H
