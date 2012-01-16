#ifndef _ARBITER_H
#define _ARBITER_H

#include <stddef.h>
#include <lib/linked_list.h>

/* data structure that describes the run-time  */
/* state of an arbiter thread */

struct arbiter_thread {
	//unix domain socket for client connections
	int monitor_sock;
	
	//list for active clients
	struct linked_list client_list;
};


#endif  //_ARBITER_H
